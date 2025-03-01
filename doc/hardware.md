
# Hardware Setup and Peripheral Design for STM32 CANBox (12V Output Signals)

This document details the hardware setup required to interface the STM32F103 microcontroller with a Peugeot 407's CAN bus and an Android head unit, *generating* 12V output signals (IGN, ILLUM, REAR) and a GND-level output (PARK) to control the head unit.

**Crucial Warning:** Incorrect connections can damage the STM32, the head unit, or both. Verify voltages and polarities *before* connecting anything. Work with automotive electronics *only* if you are comfortable and understand the risks.

## 1. Power Supply

The STM32F103 (3.3V logic) and often the CAN transceiver (3.3V or 5V) cannot be powered directly from the car's 12V battery.  We need a regulated and filtered power supply.

**Recommended:** Use an automotive-grade DC-DC buck converter (step-down converter).  A linear regulator (like LM7805) is *not* suitable.

*   **Input Voltage:** 6V to 36V (to accommodate the car's voltage fluctuations).
*   **Output Voltage:** 3.3V (for the STM32 and some transceivers) *or* 5V (for some transceivers and potentially a USB-to-UART adapter). Choose based on your component requirements.
*   **Output Current:** At least 500mA, preferably 1A or more.
*   **Features:**
    *   Over-Voltage Protection
    *   Over-Current Protection
    *   Reverse Polarity Protection
    *   Filtering

**Example DC-DC Converters:**

*   LM2596-based modules (widely available, but quality varies; some are not truly automotive-grade).
*   RECOM R-78 series (robust, industrial-grade).
*   Murata OKI-78SR series (similar to RECOM).
*   Dedicated automotive buck converters.

**Connection:**

1.  **12V Input:** Connect the converter's input to the car's 12V battery (or, *preferably*, an accessory power line switched with the ignition). **Include a fuse (e.g., 2A) in series with the 12V input for safety.**
2.  **Ground:** Connect the converter's ground to a good chassis ground point in the car.
3.  **3.3V/5V Output:** Connect the converter's output to the STM32's 3.3V or 5V pin (check your board's documentation).  Also, power the CAN transceiver and USB-to-UART adapter (if used) from this regulated output.

## 2. CAN Bus Interface

The STM32F103 has a built-in CAN controller, but it requires an external CAN transceiver.

**CAN Transceiver Options:**

*   **MCP2551:** Common, robust, 5V.  *May* require a logic level converter on the TX line when used with a 3.3V STM32 (although many use it directly without issues).
*   **SN65HVD230:** 3.3V transceiver, directly compatible with the STM32.
*   **TJA1050:** Another common 5V transceiver. Similar considerations as MCP2551.
*   **ISO1050:** Isolated CAN transceiver.  Provides galvanic isolation, highly recommended for safety and noise immunity, but more expensive.

**Connection (Example using SN65HVD230 - 3.3V):**

| SN65HVD230 Pin | Connection     | Notes                             |
| ------------- | --------------- | --------------------------------- |
| VCC           | 3.3V (from DC-DC) | Power supply.                     |
| GND           | Ground          | Common ground.                    |
| TXD           | STM32 PA12     | CAN TX.                           |
| RXD           | STM32 PA11     | CAN RX.                           |
| CANH          | Car's CAN High  | To car's CAN High wire.           |
| CANL          | Car's CAN Low   | To car's CAN Low wire.            |
| RS          | Ground          | Set to ground for high-speed mode. |

**Connection (Example using MCP2551 - 5V):**

| MCP2551 Pin | Connection             | Notes                                                                                                       |
| ----------- | ---------------------- | ----------------------------------------------------------------------------------------------------------- |
| VCC         | 5V (from DC-DC)        | Power supply.                                                                                                |
| GND         | Ground                 | Common ground.                                                                                              |
| TXD         | STM32 PA12 (CAN TX)    | Connect to the STM32's CAN TX pin.  *Ideally*, use a logic level converter (e.g., transistor circuit).       |
| RXD         | STM32 PA11 (CAN RX)    | Connect to the STM32's CAN RX pin.                                                                           |
| CANH        | Car's CAN High         | Connect to the car's CAN High wire.                                                                         |
| CANL        | Car's CAN Low          | Connect to the car's CAN Low wire.                                                                          |
| RS          | Ground                 | Set to ground for high-speed mode.                                                                           |
| VREF        | Not Connected          | (Usually not connected on breakout boards)                                                                  |

**Finding CANH and CANL:**

*   **Wiring Diagrams:** Consult wiring diagrams for your Peugeot 407 model and year.
*   **OBD-II Port:**  May provide access to *a* CAN bus (often diagnostic-only).
*   **Multimeter:**  With the car's ignition OFF, measure voltage between ground and candidate wires. CANH and CANL will typically be around 2.5V.  *Be careful not to short-circuit anything.*
*   **Oscilloscope (Ideal):** Observe the characteristic differential signals on the CAN bus.

## 3. Generating 12V Output Signals (IGN, ILLUM, REAR)

The STM32's 3.3V outputs cannot directly drive the head unit's 12V inputs. We need to use switching circuits.

**Recommended: N-Channel MOSFETs (Low-Side Switching)**

An N-channel MOSFET acts as a switch. When the STM32's GPIO is HIGH (3.3V), the MOSFET turns ON, connecting the head unit's signal input to ground (completing the circuit).  When the GPIO is LOW, the MOSFET turns OFF, and the head unit's internal pull-up resistor (presumably) pulls the signal to 12V.

*   **MOSFET:** N-channel, logic-level (Vgs(th) < 3.3V), Vds >= 20V, Id >= 100mA.
    *   Examples: 2N7000/2N7002, BS170, IRLZ44N (overkill, but readily available).
*   **Gate Resistor (Rg):** 1kΩ to 10kΩ, between STM32 GPIO and MOSFET gate.
* **Pull-Down Resistor (Optional):** A 10kOhm resistor connected between the gate of the MOSFET and the ground.

**Connection (per signal - IGN, ILLUM, REAR):**

```
STM32 GPIO Pin --- Rg --- Gate (G) of MOSFET
                          Drain (D) of MOSFET --- Head Unit Signal Input (IGN, ILLUM, or REAR)
                          Source (S) of MOSFET --- GND
```

**Alternative: Relay (Isolated, but Bulkier)**
Relays are electromechanical switches, isolating the circuits.
    *   **Relay:** 5V or 3.3V coil (match your DC-DC output), contacts rated for >= 12V and the head unit's current draw.
    *   **Transistor (NPN):** 2N3904, BC547 (to switch the relay coil).
    *   **Diode (Flyback Protection):** 1N4001, 1N4148 (across relay coil).
    *   **Resistors:** For limiting base current.

**Connection (Relay - per signal):**

```
STM32 GPIO Pin --- Rb --- Base (B) of NPN Transistor
                          Collector (C) of Transistor --- Relay Coil (-)
                          Emitter (E) of Transistor --- GND

Relay Coil (+) --- 5V (or 3.3V, depending on relay)

Relay Common (COM) --- 12V (from car)
Relay Normally Open (NO) --- Head Unit Signal Input (IGN, ILLUM, or REAR)

Diode: Anode to Relay Coil (-), Cathode to Relay Coil (+)
```

## 4. Generating GND Output Signal (PARK)
Use a P-Channel MOSFET.

*   **MOSFET:** P-channel
 *   **Vds (Drain-Source Voltage):**  At least -20V.
    *   **Id (Continuous Drain Current):**  At least 100mA.
    *   **Vgs(th) (Gate Threshold Voltage):** Must be negative
    *   **Rds(on) (Drain-Source On-Resistance):**  As low as possible.
 *   **Example MOSFETs:**
    *   IRF9Z34N
    *  SI2301CDS

*   **Gate Resistor (Rg):**  A resistor (e.g., 1kΩ to 10kΩ) between the STM32's GPIO pin and the MOSFET's gate.
* **Pull-Up Resistor:** A 10kOhm resistor connected between the gate of the MOSFET and the 3.3V power supply.

**Connection (MOSFET):**

```
STM32 GPIO Pin --- Rg --- Gate (G)
                          Drain (D) --- Head Unit Signal Input (PARK)
                          Source (S) --- GND
```

## 5. UART Connection to Android Head Unit

The STM32 communicates with the Android head unit via UART (USART1). The Android head unit likely exposes this through a USB connection (acting as a USB-to-UART bridge).

*   **STM32 PA9 (USART1_TX):** To Android head unit UART RX (or USB-to-UART adapter RX).
*   **STM32 PA10 (USART1_RX):** To Android head unit UART TX (or USB-to-UART adapter TX).
*   **Ground:** Common ground.

**Important:**

*   **Baud Rate:** 38400 (set in both STM32 code and Android head unit settings).
*   **USB-to-UART Adapter (if needed):** If the head unit doesn't have a directly accessible UART, you'll need a USB-to-UART adapter compatible with the head unit.

## Summary Table

| Signal       | STM32 Pin (Example) | Connection                                                                                         | Notes                                                                                |
| ------------- | ------------------- | -------------------------------------------------------------------------------------------------- | ------------------------------------------------------------------------------------ |
| IGN/ACC       | PB1                 | STM32 Pin -> N-Channel MOSFET Gate; MOSFET Drain to Head Unit IGN/ACC Input; MOSFET Source to GND    | Use a logic-level N-channel MOSFET.                                                  |
| ILLUM         | PB2                 | STM32 Pin -> N-Channel MOSFET Gate; MOSFET Drain to Head Unit ILLUM Input; MOSFET Source to GND    | Use a logic-level N-channel MOSFET.                                                  |
| PARK          | PB3                 | STM32 Pin -> P-Channel MOSFET Gate; MOSFET Drain to Head Unit PARK Input; MOSFET Source to GND     | Use a P-channel MOSFET.                                              |
| REAR          | PB4                 | STM32 Pin -> N-Channel MOSFET Gate; MOSFET Drain to Head Unit REAR Input; MOSFET Source to GND     | Use a logic-level N-channel MOSFET.                                                  |
| CANH          | PA11 (via transceiver) | Car's CAN High -> CAN Transceiver CANH                                                              |                                                                                      |
| CANL          | PA12 (via transceiver) | Car's CAN Low -> CAN Transceiver CANL                                                               |                                                                                      |
| UART TX       | PA9                 | STM32 PA9 -> Android Head Unit UART RX (or USB-to-UART adapter RX)                                   |                                                                                      |
| UART RX       | PA10                | STM32 PA10 -> Android Head Unit UART TX (or USB-to-UART adapter TX)                                   |                                                                                      |
| 3.3V/5V       | 3.3V/5V pin         | DC-DC Converter Output -> STM32 Power, CAN Transceiver, etc.                                      | Use a *robust*, automotive-grade DC-DC converter.                                    |
| GND           | GND pin             | Car Chassis Ground -> DC-DC Converter Ground -> STM32 Ground, etc.                                  | Ensure a common, low-impedance ground.                                               |
| 12V (Car)     | 12V source         | Car Battery (or Accessory Line) -> Fuse -> DC-DC Converter Input; Also to relay COM (if used) | Use a fuse. Preferably use an accessory line switched with the ignition.             |

This `hardware.md` provides a complete and accurate guide to the hardware setup for your STM32 CANBox project, now correctly configured to *generate* the 12V output signals required by the Android head unit. The key components are the N-channel MOSFETs for switching the 12V signals, the P-channel MOSFET for switching the GND signal, the CAN transceiver for communicating with the car, and the DC-DC converter for providing a stable power supply. Remember to always double-check your wiring and use a multimeter to verify voltages.