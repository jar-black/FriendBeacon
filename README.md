# Friend Beacon
A light beacon for friends

```mermaid
graph TD;
    A[Booting] --> B{Existing\nconfig?};
    B -- Yes --> C[Load config of\nBLE-devices];
    B -- No --> D[Create new config\nSearch for BLE-devices];
    D --> D1{More then \n3 seconds?};
    D1 -- Yes --> D2[Store config of\nfound BLE-devices];
    D2 -- Found BLE-device\nshow on ring --> D2;
    D1 -- No --> D;
    C --> E;
    D2 --> D3[Show on LED Ring\nthat config is saved];
    D3 --> E[Find any F.B.\n BLE-devices?];
    E -- Yes --> F[Show RGB\n LED Ring colors];
    F --> E;
    E -- No\nWait 5 sec --> E;
    E -- Button Pressed --> G{More then\n3 seconds?};
    G -- Yes --> D;
    G -- No --> E;
```

<h3>Parts:</h3>
- ESP32 C3 mini
- 3.7V 500mAh LiPo battery
- 3.7V LiPo battery charger
- Induction button
- RGB LED Ring
- 3D parts
  - Case
  - Diffuser
- <i>Optional, a vibration motor</i>

