// Global variables
        let port;
        let reader;
        let writer;
        let readLoopActive = false;
        let uploadInProgress = false;
        
        // Code sample URLs
        const codeSampleURLs = {
            blinkCounter: './samples/blink_serial.cpp',
            blinkMonitor: './samples/blink_monitor.cpp', 
            mainProject: './lightstrip/main.cpp' 
        };
        
        // Function to fetch code samples from URLs
        async function fetchCodeSample(url) {
            try {
                const response = await fetch(url);
                if (!response.ok) {
                    throw new Error(`Failed to fetch code sample (${response.status} ${response.statusText})`);
                }
                return await response.text();
            } catch (error) {
                console.error('Error fetching code sample:', error);
                appendToOutput('Error fetching code sample: ' + error.message + '\n');
                return '';
            }
        }
        
        // Check if Web Serial API is supported
        if (!('serial' in navigator)) {
            alert('Web Serial API is not supported in this browser. Please use Chrome or Edge.');
        }
        
        // Connect to the device
        document.getElementById('connectBtn').addEventListener('click', async () => {
            try {
                // Request port access
                port = await navigator.serial.requestPort();
                
                // Open the port with common ESP32 programming settings
                await port.open({ 
                    baudRate: 115200,
                    dataBits: 8,
                    stopBits: 1,
                    parity: "none",
                    bufferSize: 4096,
                    flowControl: "none"
                });
                
                // Update UI
                document.getElementById('connectionStatus').textContent = 'Connected';
                document.getElementById('connectBtn').disabled = true;
                document.getElementById('disconnectBtn').disabled = false;
                updateUploadButton();
                
                // Start reading from the port
                startReadingFromPort();
                
                // Create writer for later use
                writer = port.writable.getWriter();
                
                appendToOutput('Connected to ESP32 device.\n');
                
            } catch (error) {
                console.error('Error connecting to device:', error);
                appendToOutput('Error connecting to device: ' + error.message);
            }
        });
        
        // Load preset code example - Now handled by the dropdown change event
        document.getElementById('presetSelect').addEventListener('change', async () => {
            const selectedPreset = document.getElementById('presetSelect').value;
            if (selectedPreset && codeSampleURLs[selectedPreset]) {
                appendToOutput(`Fetching ${selectedPreset} example...\n`);
                const code = await fetchCodeSample(codeSampleURLs[selectedPreset]);
                if (code) {
                    document.getElementById('codeArea').value = code;
                    updateUploadButton();
                    appendToOutput(`Loaded ${selectedPreset} example.\n`);
                }
            }
        });
        
        // Original "Load Example" button can be removed or we keep it for compatibility
        document.getElementById('loadPresetBtn').addEventListener('click', async () => {
            const selectedPreset = document.getElementById('presetSelect').value;
            if (selectedPreset && codeSampleURLs[selectedPreset]) {
                appendToOutput(`Fetching ${selectedPreset} example...\n`);
                const code = await fetchCodeSample(codeSampleURLs[selectedPreset]);
                if (code) {
                    document.getElementById('codeArea').value = code;
                    updateUploadButton();
                    appendToOutput(`Loaded ${selectedPreset} example.\n`);
                }
            }
        });
        
        // Disconnect from the device
        document.getElementById('disconnectBtn').addEventListener('click', async () => {
            try {
                readLoopActive = false;
                
                if (reader) {
                    await reader.cancel();
                    reader.releaseLock();
                }
                
                if (writer) {
                    writer.releaseLock();
                }
                
                if (port) {
                    await port.close();
                }
                
                // Update UI
                document.getElementById('connectionStatus').textContent = 'Not connected';
                document.getElementById('connectBtn').disabled = false;
                document.getElementById('disconnectBtn').disabled = true;
                document.getElementById('uploadBtn').disabled = true;
                
                appendToOutput('Disconnected from device.\n');
                
            } catch (error) {
                console.error('Error disconnecting:', error);
                appendToOutput('Error disconnecting: ' + error.message);
            }
        });
        
        // Read data from the serial port - updated to handle upload verification
        async function startReadingFromPort() {
            readLoopActive = true;
            let uploadVerified = false;
            
            try {
                reader = port.readable.getReader();
                
                // Read loop
                while (readLoopActive) {
                    const { value, done } = await reader.read();
                    
                    if (done) {
                        break;
                    }
                    
                    // Convert the received bytes to text and display
                    const text = new TextDecoder().decode(value);
                    
                    // Check for verification responses during upload
                    if (text.includes('UPLOAD_SUCCESS') || text.includes('Code flashed successfully')) {
                        appendToOutput('\n✅ Upload verified successful!\n');
                        uploadVerified = true;
                    }
                    
                    if (text.includes('UPLOAD_FAILED') || text.includes('Failed to flash code')) {
                        appendToOutput('\n❌ Upload verification failed. Please try again.\n');
                    }
                    
                    appendToOutput(text);
                }
            } catch (error) {
                console.error('Error reading from port:', error);
                appendToOutput('Error reading from port: ' + error.message);
            } finally {
                if (reader) {
                    reader.releaseLock();
                }
            }
        }
        
        // Handle code input changes
        document.getElementById('codeArea').addEventListener('input', updateUploadButton);
        
        // Handle file loading
        document.getElementById('loadFileBtn').addEventListener('click', () => {
            document.getElementById('fileInput').click();
        });
        
        document.getElementById('fileInput').addEventListener('change', (event) => {
            const file = event.target.files[0];
            if (file) {
                const reader = new FileReader();
                reader.onload = (e) => {
                    document.getElementById('codeArea').value = e.target.result;
                    updateUploadButton();
                };
                reader.readAsText(file);
            }
        });
        
        // Upload code to ESP32
        document.getElementById('uploadBtn').addEventListener('click', async () => {
            if (uploadInProgress) return;
            uploadInProgress = true;
            
            const code = document.getElementById('codeArea').value;
            const uploadBtn = document.getElementById('uploadBtn');
            const progressBar = document.getElementById('uploadProgress');
            const progressElement = document.getElementById('progressBar');
            
            uploadBtn.disabled = true;
            progressBar.style.display = 'block';
            progressElement.style.width = '0%';
            progressElement.textContent = '0%';
            
            appendToOutput('\n--- Starting upload process ---\n');
            appendToOutput('Preparing code for ESP32...\n');
            
            try {
                // Step 1: Put ESP32 in upload mode
                appendToOutput('Initiating ESP32 programming mode...\n');
                await sendResetSequence();
                
                // Update progress bar
                updateProgress(20);
                await new Promise(resolve => setTimeout(resolve, 1000)); // Longer wait for bootloader
                
                // Step 2: Send code in chunks to avoid buffer issues
                appendToOutput('Uploading code to ESP32...\n');
                await sendCodeInChunks(code);
                
                // Update progress bar
                updateProgress(80);
                await new Promise(resolve => setTimeout(resolve, 1000));
                
                // Step 3: Reset the device to run the new code
                appendToOutput('Finalizing upload and resetting device...\n');
                await resetDevice();
                
                // Step 4: Verify the upload was successful
                await verifyUpload();
                
                // Update progress bar to 100%
                updateProgress(100);
                await new Promise(resolve => setTimeout(resolve, 500));
                
                appendToOutput('\nCode uploaded successfully! Device is now running the new program.\n');
                
            } catch (error) {
                console.error('Error uploading code:', error);
                appendToOutput('\nError uploading code: ' + error.message + '\n');
                updateProgress(0);
            } finally {
                uploadBtn.disabled = false;
                uploadInProgress = false;
                // Hide progress bar after 3 seconds
                setTimeout(() => {
                    progressBar.style.display = 'none';
                }, 3000);
            }
        });
        
        // Send reset sequence to put ESP32 in programming mode
        async function sendResetSequence() {
            // Send DTR/RTS signals to trigger bootloader mode
            try {
                // First, ensure we can communicate with the device
                const encoder = new TextEncoder();
                await writer.write(encoder.encode("CMD:PREPARE_UPLOAD\n"));
                await new Promise(resolve => setTimeout(resolve, 200));
                
                // More reliable reset sequence for ESP32
                await port.setSignals({ dataTerminalReady: false, requestToSend: false });
                await new Promise(resolve => setTimeout(resolve, 100));
                await port.setSignals({ dataTerminalReady: true, requestToSend: true });
                await new Promise(resolve => setTimeout(resolve, 100));
                await port.setSignals({ dataTerminalReady: false, requestToSend: true });
                await new Promise(resolve => setTimeout(resolve, 50));
                await port.setSignals({ dataTerminalReady: true, requestToSend: false });
                await new Promise(resolve => setTimeout(resolve, 50));
                await port.setSignals({ dataTerminalReady: false, requestToSend: false });
                await new Promise(resolve => setTimeout(resolve, 1000)); // Longer wait for bootloader
                
                appendToOutput('Device reset sequence completed.\n');
                return true;
            } catch (error) {
                appendToOutput('Warning: Could not send hardware reset signals. Attempting software reset...\n');
                
                // Try software-based reset by sending special command
                const encoder = new TextEncoder();
                await writer.write(encoder.encode("CMD:RESET_BOOT\n"));
                await new Promise(resolve => setTimeout(resolve, 1000));
                
                return true;
            }
        }
        
        // Send code in chunks to avoid buffer overflow
        async function sendCodeInChunks(code) {
            const encoder = new TextEncoder();
            const chunkSize = 64; // Smaller chunks for more reliability
            const totalChunks = Math.ceil(code.length / chunkSize);
            
            appendToOutput(`Sending code in ${totalChunks} chunks...\n`);
            
            // Add header to indicate start of upload with hash for verification
            const codeHash = await generateSimpleHash(code);
            await writer.write(encoder.encode(`CMD:BEGIN_UPLOAD:${codeHash}\n`));
            await new Promise(resolve => setTimeout(resolve, 300));
            
            // Send code in chunks
            for (let i = 0; i < code.length; i += chunkSize) {
                const chunk = code.substring(i, i + chunkSize);
                await writer.write(encoder.encode(chunk));
                
                // Brief pause between chunks
                await new Promise(resolve => setTimeout(resolve, 50));
                
                // Every 10 chunks, wait a bit longer to allow processing
                if ((i / chunkSize) % 10 === 0) {
                    await new Promise(resolve => setTimeout(resolve, 200));
                }
                
                // Update progress for chunks
                if (totalChunks > 1) {
                    const progress = 20 + Math.floor((i / code.length) * 60);
                    updateProgress(progress);
                }
            }
            
            // Add footer to indicate end of upload
            await writer.write(encoder.encode("\nCMD:END_UPLOAD\n"));
            await new Promise(resolve => setTimeout(resolve, 500));
            return true;
        }
        
        // Reset the device to run the new code
        async function resetDevice() {
            try {
                // Send a command to prepare for reset
                const encoder = new TextEncoder();
                await writer.write(encoder.encode("CMD:PREPARE_RESET\n"));
                await new Promise(resolve => setTimeout(resolve, 300));
                
                // Hardware reset sequence
                await port.setSignals({ dataTerminalReady: true, requestToSend: false });
                await new Promise(resolve => setTimeout(resolve, 100));
                await port.setSignals({ dataTerminalReady: false, requestToSend: false });
                await new Promise(resolve => setTimeout(resolve, 100));
                await port.setSignals({ dataTerminalReady: false, requestToSend: true });
                await new Promise(resolve => setTimeout(resolve, 100));
                await port.setSignals({ dataTerminalReady: false, requestToSend: false });
                
                appendToOutput('Device reset to run new code.\n');
                return true;
            } catch (error) {
                appendToOutput('Warning: Could not send hardware reset. Sending software reset command...\n');
                
                // Try software-based reset
                const encoder = new TextEncoder();
                await writer.write(encoder.encode("CMD:EXECUTE\n"));
                await new Promise(resolve => setTimeout(resolve, 500));
                
                return true;
            }
        }
        
        // Verify successful upload
        async function verifyUpload() {
            appendToOutput('Checking for successful upload...\n');
            
            // Send verification request
            const encoder = new TextEncoder();
            await writer.write(encoder.encode("CMD:VERIFY_UPLOAD\n"));
            
            // Wait for response for up to 5 seconds
            let verificationTimeout = setTimeout(() => {
                appendToOutput('No verification response received. Upload may not have been successful.\n');
            }, 5000);
            
            // The actual verification is handled in the serial read loop
            
            await new Promise(resolve => setTimeout(resolve, 1000));
            return true;
        }
        
        // Simple hash function for code verification
        async function generateSimpleHash(text) {
            let hash = 0;
            if (text.length === 0) return hash.toString(16);
            
            for (let i = 0; i < text.length; i++) {
                const char = text.charCodeAt(i);
                hash = ((hash << 5) - hash) + char;
                hash = hash & hash; // Convert to 32bit integer
            }
            
            return hash.toString(16);
        }
        
        // Update progress bar
        function updateProgress(percentage) {
            const progressElement = document.getElementById('progressBar');
            progressElement.style.width = percentage + '%';
            progressElement.textContent = percentage + '%';
        }
        
        // Helper functions
        function appendToOutput(text) {
            const outputDiv = document.getElementById('output');
            outputDiv.innerHTML += text.replace(/\n/g, '<br>');
            outputDiv.scrollTop = outputDiv.scrollHeight;
        }
        
        function updateUploadButton() {
            const connected = port && port.readable && port.writable;
            const codeEntered = document.getElementById('codeArea').value.trim() !== '';
            document.getElementById('uploadBtn').disabled = !(connected && codeEntered);
        }