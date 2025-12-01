const dgram = require('dgram');
const axios = require('axios');
const FormData = require('form-data');

// --- CONFIGURATION ---
// const DISCORD_WEBHOOK_URL = 'https://discord.com/api/webhooks/1440872654952206497/Fu1Saq9C2B4zQVPHxPcbDMHsbWeee5cvLWW3Iy5nIXZyZr3qLRv_EC3geUjMqHGT-91K';
const DISCORD_WEBHOOK_URL = 'https://discord.com/api/webhooks/1444909634640154654/nECYKe_anJfm8clkwXgcrnODY_5zcbsRu7yvZfa-mHbzQfVuc50weRuhGAmrjPL4DAaD';

// REPLACE with your ESP32-CAM IP address
// Most standard ESP32 firmware uses /capture for high-res stills
const ESP32_CAPTURE_URL = 'http://192.168.4.1/still'; 

const UDP_SERVER_PORT = 7070;
const UDP_SERVER_ADDRESS = '127.0.0.1'; 

// --- DISCORD FUNCTION WITH IMAGE ---
const sendDiscordAlert = async (message) => {
    console.log(`🚨 Alert Received: "${message}". Fetching camera snapshot...`);

    try {
        // 1. Get the image from the ESP32-CAM
        // We request a 'stream' so we can pipe it directly to Discord without saving it to a file first
        const cameraResponse = await axios.get(ESP32_CAPTURE_URL, {
            responseType: 'stream',
            timeout: 2000 // 2 second timeout in case camera is offline
        });

        // 2. Prepare the Form Data for Discord
        const form = new FormData();
        
        // Attach the image stream as 'file'
        form.append('file', cameraResponse.data, 'snapshot.jpg');

        // Attach the JSON payload (the text/embed part)
        const payload = {
            username: 'BeagleY Security',
            embeds: [{
                title: "🚨 Motion Detected!",
                description: message,
                color: 15158332, // Red
                image: {
                    url: "attachment://snapshot.jpg" // References the file we just attached
                },
                footer: { text: `BeagleY-AI • ${new Date().toLocaleTimeString()}` }
            }]
        };

        form.append('payload_json', JSON.stringify(payload));

        // 3. Send to Discord
        await axios.post(DISCORD_WEBHOOK_URL, form, {
            headers: {
                ...form.getHeaders() // This is crucial for multipart/form-data
            }
        });

        console.log('✅ Snapshot sent to Discord!');

    } catch (error) {
        console.error('❌ Error:', error.message);
        // If camera fails, send a text-only alert so you still know something happened
        if (error.code === 'ECONNREFUSED' || error.code === 'ETIMEDOUT') {
            sendTextOnlyFallback(message, "Camera unreachable");
        }
    }
};

// Fallback function if camera is offline
const sendTextOnlyFallback = async (message, errorDetail) => {
    try {
        await axios.post(DISCORD_WEBHOOK_URL, {
            content: `⚠️ **Alert:** ${message}\n(Camera snapshot failed: ${errorDetail})`
        });
    } catch (e) { console.error("Failed to send fallback message"); }
};

// --- UDP SERVER SETUP ---
const server = dgram.createSocket('udp4');

server.on('message', (msg, rinfo) => {
    const receivedMessage = msg.toString().trim();
    console.log(`UDP Trigger from ${rinfo.address}: ${receivedMessage}`);
    
    // Fire the main alert logic
    sendDiscordAlert(receivedMessage);
});

server.on('listening', () => {
    const address = server.address();
    console.log(`📡 Security System Listening on ${address.port}`);
    console.log(`📷 Camera Target: ${ESP32_CAPTURE_URL}`);
});

server.bind(UDP_SERVER_PORT, UDP_SERVER_ADDRESS);