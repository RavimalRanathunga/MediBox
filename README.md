# MediBox

MediBox is a smart medication box system created using an ESP32 microcontroller, integrated with a Node-RED dashboard for visualization and control.

## Features

- ESP32-based hardware for controlling and monitoring medication dispensing
- Node-RED dashboard for real-time data visualization and management
- Customizable and extendable flows using Node-RED

## Getting Started

### 1. Clone the Repository

Open your terminal and run:

```bash
git clone https://github.com/RavimalRanathunga/MediBox.git
cd MediBox
```

### 2. Install Node-RED

You need Node.js installed first. Then, install Node-RED globally:

```bash
npm install -g --unsafe-perm node-red
```

Or follow the [official Node-RED installation guide](https://nodered.org/docs/getting-started/).

### 3. Start Node-RED

In your terminal, simply run:

```bash
node-red
```

Node-RED will start and typically be accessible at [http://localhost:1880](http://localhost:1880).

### 4. Import `flows.json` to Node-RED

1. Open the Node-RED editor in your browser: [http://localhost:1880](http://localhost:1880)
2. Click the menu (☰) in the top right corner, then select **Import**.
3. Click **Select a file to import**, then choose the `flows.json` file from this repository.
4. Click **Import** to add the flows to your workspace.
5. Click **Deploy** to activate the flows.

### 5. Flash the ESP32

Upload your ESP32 code (located in the repository) to your ESP32 board using Arduino IDE or your preferred method.

---

## Project Structure

- `flows.json` &mdash; Node-RED flow definitions for the dashboard and logic
- `ESP32_Code/` &mdash; Source code for the ESP32 microcontroller

## Dependencies

- Node.js
- Node-RED
- ESP32 libraries (for Arduino or PlatformIO)

## License

This project is licensed under the MIT License.

---

## Contribution

Feel free to submit issues or pull requests to improve the project!

---

Let me know if you want to include sample ESP32 code setup or dashboard screenshots.
