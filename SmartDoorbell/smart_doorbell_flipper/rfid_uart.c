#include <furi.h>
#include <furi_hal.h>
#include <gui/gui.h>
#include <lfrfid/lfrfid_worker.h>
#include <lfrfid/protocols/lfrfid_protocols.h>

// --- CONFIG ---
#define UART_CH FuriHalSerialIdUsart
#define BAUDRATE 9600

// --- EVENTS ---
typedef enum {
    EventTypeTick,
    EventTypeKey,
    EventTypeRfidRead
} EventType;

typedef struct {
    EventType type;
    InputEvent input; // For button presses
    // We can use a fixed buffer for the RFID ID to pass it to main thread
    char rfid_data[32]; 
} AppEvent;

// --- APP STATE ---
typedef struct {
    LFRFIDWorker* lfrfid_worker;
    ProtocolDict* dict;
    Gui* gui;
    ViewPort* view_port;
    FuriHalSerialHandle* serial_handle;
    FuriMessageQueue* event_queue; // The magic fix
    FuriString* temp_str;
} RfidUartApp;

// --- UART SENDER ---
void send_uart_string(RfidUartApp* app, const char* str) {
    if(app->serial_handle) {
        furi_hal_serial_tx(app->serial_handle, (uint8_t*)str, strlen(str));
        furi_hal_serial_tx(app->serial_handle, (uint8_t*)"\r\n", 2);
    }
}

// --- RFID CALLBACK (Runs in Worker Thread) ---
// We only notify the main thread here. No heavy lifting!
static void lfrfid_read_callback(LFRFIDWorkerReadResult result, ProtocolId protocol, void* context) {
    RfidUartApp* app = context;

    if(result == LFRFIDWorkerReadDone) {
        // Extract data immediately while worker has it
        size_t data_size = protocol_dict_get_data_size(app->dict, protocol);
        
        if(data_size > 0 && data_size < 10) { // Sanity check size
            uint8_t* data = malloc(data_size);
            protocol_dict_get_data(app->dict, protocol, data, data_size);

            AppEvent event;
            event.type = EventTypeRfidRead;
            // Format to hex string in the event struct
            event.rfid_data[0] = '\0'; // clear
            
            // Manual Hex formatting to avoid heavy furi_string in callback if possible, 
            // but for simplicity we assume single threaded string usage is risky, 
            // so we do raw bytes -> hex char conversion loop.
            char* ptr = event.rfid_data;
            for(size_t i = 0; i < data_size; i++) {
                ptr += snprintf(ptr, 32 - (ptr - event.rfid_data), "%02X", data[i]);
            }
            free(data);

            // Send to Main Thread
            furi_message_queue_put(app->event_queue, &event, 0);
        }
    }
}

// --- INPUT CALLBACK ---
static void input_callback(InputEvent* input_event, void* ctx) {
    RfidUartApp* app = ctx;
    AppEvent event;
    event.type = EventTypeKey;
    event.input = *input_event;
    furi_message_queue_put(app->event_queue, &event, 0);
}

// --- GUI RENDER ---
static void render_callback(Canvas* canvas, void* ctx) {
    RfidUartApp* app = ctx;
    canvas_clear(canvas);
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str(canvas, 2, 12, "RFID -> UART Bridge");
    
    canvas_set_font(canvas, FontSecondary);
    canvas_draw_str(canvas, 2, 24, "Scanning...");
    canvas_draw_str(canvas, 2, 36, "TX: Pin 13  RX: Pin 14");
    
    if (furi_string_size(app->temp_str) > 0) {
        canvas_draw_str(canvas, 2, 50, "Sent:");
        canvas_draw_str(canvas, 30, 50, furi_string_get_cstr(app->temp_str));
    }
}

// --- MAIN ENTRY ---
int32_t rfid_uart_app(void* p) {
    UNUSED(p);
    RfidUartApp* app = malloc(sizeof(RfidUartApp));
    app->temp_str = furi_string_alloc();
    app->event_queue = furi_message_queue_alloc(8, sizeof(AppEvent));

    // 1. Init UART
    app->serial_handle = furi_hal_serial_control_acquire(UART_CH);
    if(app->serial_handle) {
        furi_hal_serial_init(app->serial_handle, BAUDRATE);
    }

    // 2. Init GUI
    app->gui = furi_record_open(RECORD_GUI);
    app->view_port = view_port_alloc();
    view_port_draw_callback_set(app->view_port, render_callback, app);
    view_port_input_callback_set(app->view_port, input_callback, app);
    gui_add_view_port(app->gui, app->view_port, GuiLayerFullscreen);

    // 3. Init RFID
    app->dict = protocol_dict_alloc(lfrfid_protocols, LFRFIDProtocolMax);
    app->lfrfid_worker = lfrfid_worker_alloc(app->dict);
    
    // Start the thread ONCE
    lfrfid_worker_start_thread(app->lfrfid_worker);
    lfrfid_worker_read_start(app->lfrfid_worker, LFRFIDWorkerReadTypeAuto, lfrfid_read_callback, app);

    // 4. Main Event Loop
    AppEvent event;
    bool running = true;
    while(running) {
        // Wait for event
        FuriStatus status = furi_message_queue_get(app->event_queue, &event, FuriWaitForever);
        
        if(status == FuriStatusOk) {
            if(event.type == EventTypeKey) {
                if(event.input.key == InputKeyBack && event.input.type == InputTypeShort) {
                    running = false;
                }
            }
            else if(event.type == EventTypeRfidRead) {
                // HANDLE RFID SUCCESS HERE (Main Thread Context)
                
                // 1. Update Screen Buffer
                furi_string_set(app->temp_str, event.rfid_data);
                view_port_update(app->view_port);

                // 2. Send UART
                FURI_LOG_I("RFID", "Sent: %s", event.rfid_data);
                send_uart_string(app, event.rfid_data);
                
                // 3. Feedback
                furi_hal_vibro_on(true);
                furi_delay_ms(100);
                furi_hal_vibro_on(false);

                // 4. Reset Logic (Pause -> Wait -> Resume)
                // We use 'lfrfid_worker_stop' to stop SCANNING, not the thread.
                lfrfid_worker_stop(app->lfrfid_worker); 
                
                // Wait 1 second to prevent spamming
                furi_delay_ms(1000); 
                
                // Resume scanning
                lfrfid_worker_read_start(app->lfrfid_worker, LFRFIDWorkerReadTypeAuto, lfrfid_read_callback, app);
            }
        }
    }

    // 5. Cleanup
    lfrfid_worker_stop(app->lfrfid_worker);
    lfrfid_worker_stop_thread(app->lfrfid_worker); // Stop thread ONLY at exit
    lfrfid_worker_free(app->lfrfid_worker);
    protocol_dict_free(app->dict);

    gui_remove_view_port(app->gui, app->view_port);
    view_port_free(app->view_port);
    furi_record_close(RECORD_GUI);

    if(app->serial_handle) {
        furi_hal_serial_deinit(app->serial_handle);
        furi_hal_serial_control_release(app->serial_handle);
    }

    furi_message_queue_free(app->event_queue);
    furi_string_free(app->temp_str);
    free(app);
    return 0;
}