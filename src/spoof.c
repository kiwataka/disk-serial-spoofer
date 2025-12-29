#include <ntddk.h>
#include <ntddstor.h>

// Declaration of undocumented RtlRandomEx (available since Windows XP)
NTSYSAPI ULONG RtlRandomEx(PULONG Seed);

typedef struct _DEVICE_EXTENSION {
    PDEVICE_OBJECT LowerDevice;
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

// Generate random serial number in format XXXX_XXXX_XXXX_XXXX
VOID GenSerial(PCHAR buf) {
    ULONG seed = 0;
    ULONG r = RtlRandomEx(&seed);
    PCHAR p = buf;

    for (int i = 0; i < 16; i++) {
        if (i > 0 && i % 4 == 0) {
            *p++ = '_';
        }
        UCHAR nibble = (r >> ((15 - i) * 4)) & 0xF;
        *p++ = (nibble < 10 ? '0' + nibble : 'A' + nibble - 10);
    }
    *p = '\0';
}

// Completion routine — serial number spoofing happens here
NTSTATUS OnComplete(PDEVICE_OBJECT DeviceObject, PIRP Irp, PVOID Context) {
    UNREFERENCED_PARAMETER(Context);

    PDEVICE_EXTENSION ext = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;

    if (NT_SUCCESS(Irp->IoStatus.Status) && Irp->IoStatus.Information > 0) {
        PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);

        if (irpSp->Parameters.DeviceIoControl.IoControlCode == IOCTL_STORAGE_QUERY_PROPERTY) {
            PSTORAGE_DEVICE_DESCRIPTOR desc = (PSTORAGE_DEVICE_DESCRIPTOR)Irp->AssociatedIrp.SystemBuffer;

            if (desc && desc->SerialNumberOffset && desc->SerialNumberOffset < Irp->IoStatus.Information) {
                PCHAR sn = (PCHAR)desc + desc->SerialNumberOffset;
                GenSerial(sn); // Replace original serial with random one
            }
        }
    }

    IoSkipCurrentIrpStackLocation(Irp);
    return IoCallDriver(ext->LowerDevice, Irp);
}

// Handle IRP_MJ_DEVICE_CONTROL — intercept disk queries
NTSTATUS DispatchDeviceControl(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);

    if (irpSp->Parameters.DeviceIoControl.IoControlCode == IOCTL_STORAGE_QUERY_PROPERTY) {
        IoCopyCurrentIrpStackLocationToNext(Irp);
        IoSetCompletionRoutine(Irp, OnComplete, NULL, TRUE, TRUE, TRUE);
        return IoCallDriver(((PDEVICE_EXTENSION)DeviceObject->DeviceExtension)->LowerDevice, Irp);
    }

    IoSkipCurrentIrpStackLocation(Irp);
    return IoCallDriver(((PDEVICE_EXTENSION)DeviceObject->DeviceExtension)->LowerDevice, Irp);
}

// Attach filter to physical disk device
NTSTATUS AddDevice(PDRIVER_OBJECT DriverObject, PDEVICE_OBJECT PhysicalDeviceObject) {
    PDEVICE_OBJECT filterDeviceObject = NULL;
    NTSTATUS status = IoCreateDevice(
        DriverObject,
        sizeof(DEVICE_EXTENSION),
        NULL,
        FILE_DEVICE_DISK,
        0,
        FALSE,
        &filterDeviceObject
    );

    if (!NT_SUCCESS(status)) {
        return status;
    }

    PDEVICE_EXTENSION ext = (PDEVICE_EXTENSION)filterDeviceObject->DeviceExtension;
    ext->LowerDevice = IoAttachDeviceToDeviceStack(filterDeviceObject, PhysicalDeviceObject);

    if (!ext->LowerDevice) {
        IoDeleteDevice(filterDeviceObject);
        return STATUS_DEVICE_REMOVED;
    }

    filterDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
    filterDeviceObject->Flags |= (ext->LowerDevice->Flags & (DO_DIRECT_IO | DO_BUFFERED_IO | DO_POWER_PAGABLE));

    return STATUS_SUCCESS;
}

// Default handler for unimplemented IRP types
NTSTATUS DefaultDispatch(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
    UNREFERENCED_PARAMETER(DeviceObject);
    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

// Driver entry point
NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath) {
    UNREFERENCED_PARAMETER(RegistryPath);

    // Set handler only for DeviceControl
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DispatchDeviceControl;

    // Pass through all other IRPs
    for (ULONG i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; ++i) {
        if (DriverObject->MajorFunction[i] == NULL) {
            DriverObject->MajorFunction[i] = DefaultDispatch;
        }
    }

    // Register filter attachment
    DriverObject->DriverExtension->AddDevice = AddDevice;

    return STATUS_SUCCESS;
}
