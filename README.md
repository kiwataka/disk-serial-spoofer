<div align="center">

# 💾 Disk Serial Spoofer Driver

<img src="https://img.shields.io/badge/Windows_11_23H2-0078D6?style=for-the-badge&logo=windows&logoColor=white" alt="Windows 11 23H2"/>
<img src="https://img.shields.io/badge/Kernel_Mode_Driver-000000?style=for-the-badge&logo=c&logoColor=white" alt="Kernel Mode"/>
<img src="https://img.shields.io/badge/HWID_Spoofing-FF4500?style=for-the-badge&logo=shield&logoColor=white" alt="HWID Spoofing"/>
<img src="https://img.shields.io/badge/Filter_Driver-32CD32?style=for-the-badge&logo=windows&logoColor=white" alt="Filter Driver"/>

<br/><br/>

**Advanced kernel-mode filter driver that dynamically spoofs physical disk serial numbers**

Generates a unique random serial on every query — essential component for complete hardware identity randomization.

</div>

<br/>

### 🔧 Key Features

- Intercepts `IOCTL_STORAGE_QUERY_PROPERTY` via completion routine
- Generates cryptographically strong 16-character serial in format `XXXX_XXXX_XXXX_XXXX`
- Uses undocumented kernel function `RtlRandomEx` for high-quality randomness
- Transparent upper-filter attachment to all disk class devices
- Compatible with NVMe, SATA, SCSI and virtual disks
- Fully in-memory operation — no persistent changes to disk
- Extremely low performance overhead

<br/>

### ⚠️ Compatibility & Important Warnings

- **Works exclusively on Windows 11 23H2 (build 22631)**
- Requires **Test Signing mode** enabled or Driver Signature Enforcement disabled
- **For educational and research purposes only**
- Not intended for production environments or bypassing anti-cheat systems
- Incorrect installation may cause system instability — use at your own risk

<br/>

### 📦 Repository Contents

- `spoof.c` — Complete driver source code (filter logic, random serial generation, completion routine)
- `spoof.inf` — INF installation file for easy driver deployment

<br/>

### 🛠 Build & Installation Guide

#### Prerequisites
- Windows 11 23H2 (x64)
- Windows Driver Kit (WDK) + Visual Studio
- Administrator rights

#### 1. Build the driver

Open **x64 Native Tools Command Prompt for VS** and run:

```cmd
cd path\to\project
build -cegz

Result: `spoof.sys`

#### 2. Enable Test Signing (one-time setup)

```cmd
bcdedit /set testsigning on

Reboot the system.

#### 3. Install the driver

**Method A — Using INF (recommended)**

```cmd
copy spoof.sys C:\Windows\System32\drivers\
copy spoof.inf C:\Windows\System32\

**Method B — Manual service**

```cmd
sc create spoof type= kernel start= demand binPath= C:\Windows\System32\drivers\spoof.sys
sc start spoof

#### 4. Verify spoofing

1. Open **Device Manager** → **Disk drives**
2. Right-click any disk → **Properties** → **Details** tab
3. Select **Hardware IDs** or **Device instance path**

You should now see a random serial number like `A3F1_9E7D_2C4B_8F06`.  
The serial changes on every driver reload or system reboot.

#### 5. Unload the driver (when finished)

```cmd
sc stop spoof
sc delete spoof
bcdedit /set testsigning off

Reboot required.

<br/>

### 🔬 How It Works

The driver acts as an upper filter for the disk class.  
When the operating system requests disk properties via `IOCTL_STORAGE_QUERY_PROPERTY`, the completion routine intercepts the response and overwrites the original serial number field with a newly generated random string.

All operations occur in memory — no modifications to the actual disk.

<br/>

<div align="center">

**Advanced low-level tool for hardware identity research**  

Contact: [@kiwataka](https://t.me/kiwataka) | Discord: k1wataka

<br/><br/>

<img src="https://img.shields.io/badge/Educational_Use_Only-FF0000?style=for-the-badge&logo=science&logoColor=white"/>
<img src="https://img.shields.io/badge/No_Warranty-333333?style=for-the-badge"/>

</div>
