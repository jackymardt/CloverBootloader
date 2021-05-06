/** @file
  Commonly used kext and kernel patches.

Copyright (c) 2018, vit9696. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Base.h>

#include <IndustryStandard/AppleIntelCpuInfo.h>
#include <Library/BaseMemoryLib.h>
#include <Library/OcDebugLogLib.h>
#include <Library/OcAppleKernelLib.h>
#include <Library/PrintLib.h>
#include <Library/OcFileLib.h>
#include <Library/OcStringLib.h>
#include <Library/UefiLib.h>

STATIC CONST UINT8 mMovEcxE2[] = {
  0xB9, 0xE2, 0x00, 0x00, 0x00 // mov ecx, 0E2h
};

STATIC CONST UINT8 mMovCxE2[] = {
  0x66, 0xB9, 0xE2, 0x00 // mov cx, 0E2h
};

STATIC CONST UINT8 mWrmsr[] = {
  0x0F, 0x30 // wrmsr
};

STATIC CONST UINTN mWrmsrMaxDistance = 32;

STATIC
EFI_STATUS
PatchAppleCpuPmCfgLock (
  IN OUT PATCHER_CONTEXT    *Patcher,
  IN     UINT32             KernelVersion
  )
{
  UINTN   Count;
  UINT8   *Walker;
  UINT8   *WalkerEnd;
  UINT8   *WalkerTmp;

  if (Patcher == NULL) {
    return EFI_NOT_FOUND;
  }

  Count = 0;
  Walker = (UINT8 *) MachoGetMachHeader64 (&Patcher->MachContext);
  WalkerEnd = Walker + MachoGetFileSize (&Patcher->MachContext) - mWrmsrMaxDistance;

  //
  // Thx to Clover developers for the approach.
  //
  while (Walker < WalkerEnd) {
    //
    // Match (e)cx 0E2h assignment.
    //
    if (Walker[0] == mMovEcxE2[0]
      && Walker[1] == mMovEcxE2[1]
      && Walker[2] == mMovEcxE2[2]
      && Walker[3] == mMovEcxE2[3]
      && Walker[4] == mMovEcxE2[4]) {
      STATIC_ASSERT (sizeof (mMovEcxE2) == 5, "Unsupported mMovEcxE2");
      Walker += sizeof (mMovEcxE2);
    } else if (Walker[0] == mMovCxE2[0]
      && Walker[1] == mMovCxE2[1]
      && Walker[2] == mMovCxE2[2]
      && Walker[3] == mMovCxE2[3]) {
      STATIC_ASSERT (sizeof (mMovCxE2) == 4, "Unsupported mMovCxE2");
      Walker += sizeof (mMovCxE2);
    } else {
      ++Walker;
      continue;
    }

    WalkerTmp = Walker + mWrmsrMaxDistance;

    while (Walker < WalkerTmp) {
      if (Walker[0] == mWrmsr[0]
        && Walker[1] == mWrmsr[1]) {
        STATIC_ASSERT (sizeof (mWrmsr) == 2, "Unsupported mWrmsr");
        ++Count;
        //
        // Patch matched wrmsr with nop.
        //
        *Walker++ = 0x90;
        *Walker++ = 0x90;
        break;
      }

      if ((Walker[0] == 0xC9 && Walker[1] == 0xC3) ///< leave; ret
        || (Walker[0] == 0x5D && Walker[1] == 0xC3)) { ///< pop rbp; ret
        //
        // Stop searching upon matching return sequences.
        //
        Walker += 2;
        break;
      }

      if ((Walker[0] == 0xB9 && Walker[3] == 0 && Walker[4] == 0) ///< mov ecx, 00000xxxxh
        || (Walker[0] == 0x66 && Walker[1] == 0xB9 && Walker[3] == 0)) { ///< mov cx, 00xxh
        //
        // Stop searching upon matching reassign sequences.
        //
        break;
      }

      //
      // Continue searching.
      //
      ++Walker;
    }
  }
  
  //
  // At least one patch must be successful for this to work.
  //
  return Count > 0 ? EFI_SUCCESS : EFI_NOT_FOUND;
}

#pragma pack(push, 1)

//
// XCPM record definition, extracted from XNU debug kernel.
//
typedef struct XCPM_MSR_RECORD_ {
  UINT32  xcpm_msr_num;
  UINT32  xcpm_msr_applicable_cpus;
  UINT32  *xcpm_msr_flag_p;
  UINT64  xcpm_msr_bits_clear;
  UINT64  xcpm_msr_bits_set;
  UINT64  xcpm_msr_initial_value;
  UINT64  xcpm_msr_rb_value;
} XCPM_MSR_RECORD;

#pragma pack(pop)

STATIC
UINT8
mXcpmCfgLockRelFind[] = {
  0xB9, 0xE2, 0x00, 0x00, 0x00, 0x0F, 0x30 // mov ecx, 0xE2 ; wrmsr
};

STATIC
UINT8
mXcpmCfgLockRelReplace[] = {
  0xB9, 0xE2, 0x00, 0x00, 0x00, 0x90, 0x90 // mov ecx, 0xE2 ; nop
};

STATIC
PATCHER_GENERIC_PATCH
mXcpmCfgLockRelPatch = {
  .Comment     = DEBUG_POINTER ("XcpmCfgLockRel"),
  .Base        = "_xcpm_idle",
  .Find        = mXcpmCfgLockRelFind,
  .Mask        = NULL,
  .Replace     = mXcpmCfgLockRelReplace,
  .ReplaceMask = NULL,
  .Size        = sizeof (mXcpmCfgLockRelFind),
  .Count       = 2,
  .Skip        = 0,
  .Limit       = 4096
};

STATIC
UINT8
mXcpmCfgLockDbgFind[] = {
  0xBF, 0xE2, 0x00, 0x00, 0x00, 0xE8 // mov edi, 0xE2 ; call (wrmsr64)
};

STATIC
UINT8
mXcpmCfgLockDbgReplace[] = {
  0xEB, 0x08, 0x90, 0x90, 0x90, 0xE8 // jmp LBL ; nop; nop; nop; call (wrmsr64); LBL:
};

STATIC
PATCHER_GENERIC_PATCH
mXcpmCfgLockDbgPatch = {
  .Comment     = DEBUG_POINTER ("XcpmCfgLockDbg"),
  .Base        = "_xcpm_cst_control_evaluate",
  .Find        = mXcpmCfgLockDbgFind,
  .Mask        = NULL,
  .Replace     = mXcpmCfgLockDbgReplace,
  .ReplaceMask = NULL,
  .Size        = sizeof (mXcpmCfgLockDbgFind),
  .Count       = 2,
  .Skip        = 0,
  .Limit       = 4096
};

STATIC
EFI_STATUS
PatchAppleXcpmCfgLock (
  IN OUT PATCHER_CONTEXT    *Patcher,
  IN     UINT32             KernelVersion
  )
{
  EFI_STATUS          Status;
  XCPM_MSR_RECORD     *Record;
  XCPM_MSR_RECORD     *Last;

  UINT32              Replacements;

  ASSERT (Patcher != NULL);

  if (!OcMatchDarwinVersion (KernelVersion, KERNEL_VERSION_MOUNTAIN_LION_MIN, 0)) {
    DEBUG ((DEBUG_INFO, "OCAK: Skipping XcpmCfgLock on %u\n", KernelVersion));
    return EFI_SUCCESS;
  }

  Last = (XCPM_MSR_RECORD *) ((UINT8 *) MachoGetMachHeader64 (&Patcher->MachContext)
    + MachoGetFileSize (&Patcher->MachContext) - sizeof (XCPM_MSR_RECORD));

  Replacements = 0;

  Status = PatcherGetSymbolAddress (Patcher, "_xcpm_core_scope_msrs", (UINT8 **) &Record);
  if (!EFI_ERROR (Status)) {
    while (Record < Last) {
      if (Record->xcpm_msr_num == 0xE2) {
        DEBUG ((
          DEBUG_INFO,
          "OCAK: Replacing _xcpm_core_scope_msrs data %u %u\n",
          Record->xcpm_msr_num,
          Record->xcpm_msr_applicable_cpus
          ));
        Record->xcpm_msr_applicable_cpus = 0;
        ++Replacements;
      } else {
        DEBUG ((
          DEBUG_INFO,
          "OCAK: Not matching _xcpm_core_scope_msrs data %u %u\n",
          Record->xcpm_msr_num,
          Record->xcpm_msr_applicable_cpus
          ));
        break;
      }
      ++Record;
    }

    //
    // Now the HWP patch at _xcpm_idle() for Release XNU.
    //
    Status = PatcherApplyGenericPatch (
      Patcher,
      &mXcpmCfgLockRelPatch
      );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "OCAK: Failed to locate _xcpm_idle release patch - %r, trying dbg\n", Status));
      Status = PatcherApplyGenericPatch (
        Patcher,
        &mXcpmCfgLockDbgPatch
        );
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_WARN, "OCAK: Failed to locate _xcpm_idle patches - %r\n", Status));
      }
    }
  } else {
    DEBUG ((DEBUG_WARN, "OCAK: Failed to locate _xcpm_core_scope_msrs - %r\n", Status));
  }

  return Replacements > 0 ? EFI_SUCCESS : EFI_NOT_FOUND;
}

STATIC
UINT8
mMiscPwrMgmtRelFind[] = {
  0xB9, 0xAA, 0x01, 0x00, 0x00, 0x0F, 0x30 // mov ecx, 0x1aa; wrmsr
};

STATIC
UINT8
mMiscPwrMgmtRelReplace[] = {
  0xB9, 0xAA, 0x01, 0x00, 0x00, 0x90, 0x90 // mov ecx, 0x1aa; nop
};

STATIC
PATCHER_GENERIC_PATCH
mMiscPwrMgmtRelPatch = {
  .Comment     = DEBUG_POINTER ("MiscPwrMgmtRel"),
  .Base        = NULL,
  .Find        = mMiscPwrMgmtRelFind,
  .Mask        = NULL,
  .Replace     = mMiscPwrMgmtRelReplace,
  .ReplaceMask = NULL,
  .Size        = sizeof (mMiscPwrMgmtRelFind),
  .Count       = 0,
  .Skip        = 0,
  .Limit       = 0
};

STATIC
UINT8
mMiscPwrMgmtDbgFind[] = {
  0xBF, 0xAA, 0x01, 0x00, 0x00, 0xE8 // mov edi, 0x1AA ; call (wrmsr64)
};

STATIC
UINT8
mMiscPwrMgmtDbgReplace[] = {
  0xEB, 0x08, 0x90, 0x90, 0x90, 0xE8 // jmp LBL ; nop; nop; nop; call (wrmsr64); LBL:
};

STATIC
PATCHER_GENERIC_PATCH
mMiscPwrMgmtDbgPatch = {
  //
  // NOTE: This substitution is going to take place
  //       at both _xcpm_hwp_enable()
  //       and _xcpm_enable_hw_coordination() (which is inlined in Release XNU).
  //
  .Comment     = DEBUG_POINTER ("MiscPwrMgmtDbg"),
  .Base        = NULL,
  .Find        = mMiscPwrMgmtDbgFind,
  .Mask        = NULL,
  .Replace     = mMiscPwrMgmtDbgReplace,
  .ReplaceMask = NULL,
  .Size        = sizeof (mMiscPwrMgmtDbgFind),
  .Count       = 0,
  .Skip        = 0,
  .Limit       = 0
};

STATIC
EFI_STATUS
PatchAppleXcpmExtraMsrs (
  IN OUT PATCHER_CONTEXT    *Patcher,
  IN     UINT32             KernelVersion
  )
{
  EFI_STATUS          Status;
  XCPM_MSR_RECORD     *Record;
  XCPM_MSR_RECORD     *Last;
  UINT32              Replacements;

  ASSERT (Patcher != NULL);

  if (!OcMatchDarwinVersion (KernelVersion, KERNEL_VERSION_MOUNTAIN_LION_MIN, 0)) {
    DEBUG ((DEBUG_INFO, "OCAK: Skipping XcpmExtraMsrs on %u\n", KernelVersion));
    return EFI_SUCCESS;
  }

  Last = (XCPM_MSR_RECORD *) ((UINT8 *) MachoGetMachHeader64 (&Patcher->MachContext)
    + MachoGetFileSize (&Patcher->MachContext) - sizeof (XCPM_MSR_RECORD));

  Replacements = 0;

  Status = PatcherGetSymbolAddress (Patcher, "_xcpm_pkg_scope_msrs", (UINT8 **) &Record);
  if (!EFI_ERROR (Status)) {
    while (Record < Last) {
      //
      // Most Record->xcpm_msr_applicable_cpus has
      // 0xDC or 0xDE in its lower 16-bit and thus here we
      // AND 0xFF0000FDU in order to match both. (The result will be 0xDC)
      //
      if ((Record->xcpm_msr_applicable_cpus & 0xFF0000FDU) == 0xDC) {
        DEBUG ((
          DEBUG_INFO,
          "OCAK: Replacing _xcpm_pkg_scope_msrs data %u %u\n",
          Record->xcpm_msr_num,
          Record->xcpm_msr_applicable_cpus
          ));
        Record->xcpm_msr_applicable_cpus = 0;
        ++Replacements;
      } else {
        DEBUG ((
          DEBUG_INFO,
          "OCAK: Not matching _xcpm_pkg_scope_msrs data %u %u\n",
          Record->xcpm_msr_num,
          Record->xcpm_msr_applicable_cpus
          ));
        break;
      }
      ++Record;
    }
  } else {
    DEBUG ((DEBUG_WARN, "OCAK: Failed to locate _xcpm_pkg_scope_msrs - %r\n", Status));
  }

  Status = PatcherGetSymbolAddress (Patcher, "_xcpm_SMT_scope_msrs", (UINT8 **) &Record);
  if (!EFI_ERROR (Status)) {
    while (Record < Last) {
      if (Record->xcpm_msr_flag_p == NULL) {
        DEBUG ((
          DEBUG_INFO,
          "OCAK: Replacing _xcpm_SMT_scope_msrs data %u %u\n",
          Record->xcpm_msr_num,
          Record->xcpm_msr_applicable_cpus
          ));
        Record->xcpm_msr_applicable_cpus = 0;
        ++Replacements;
      } else {
        DEBUG ((
          DEBUG_INFO,
          "OCAK: Not matching _xcpm_SMT_scope_msrs data %u %u %p\n",
          Record->xcpm_msr_num,
          Record->xcpm_msr_applicable_cpus,
          Record->xcpm_msr_flag_p
          ));
        break;
      }
      ++Record;
    }
  } else {
    DEBUG ((DEBUG_WARN, "OCAK: Failed to locate _xcpm_SMT_scope_msrs - %r\n", Status));
  }

  //
  // Now patch writes to MSR_MISC_PWR_MGMT
  //
  Status = PatcherApplyGenericPatch (Patcher, &mMiscPwrMgmtRelPatch);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCAK: Failed to patch writes to MSR_MISC_PWR_MGMT - %r, trying dbg\n", Status));
    Status = PatcherApplyGenericPatch (Patcher, &mMiscPwrMgmtDbgPatch);
  }

  if (!EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCAK: Patched writes to MSR_MISC_PWR_MGMT\n"));
    ++Replacements;
  } else {
    DEBUG ((DEBUG_WARN, "OCAK: Failed to patch writes to MSR_MISC_PWR_MGMT - %r\n", Status));
  }

  return Replacements > 0 ? EFI_SUCCESS : EFI_NOT_FOUND;
}

STATIC
UINT8
mPerfCtrlFind1[] = {
  0xB9, 0x99, 0x01, 0x00, 0x00, ///< mov ecx, 199h
  0x0F, 0x30                    ///< wrmsr
};

STATIC
UINT8
mPerfCtrlFind2[] = {
  0xB9, 0x99, 0x01, 0x00, 0x00, ///< mov ecx, 199h
  0x31, 0xD2,                   ///< xor edx, edx
  0x0F, 0x30                    ///< wrmsr
};

STATIC
UINT8
mPerfCtrlMax[] = {
  0xB9, 0x99, 0x01, 0x00, 0x00, ///< mov ecx, 199h
  0x31, 0xD2,                   ///< xor edx, edx
  0xB8, 0x00, 0xFF, 0x00, 0x00, ///< mov eax, 0xFF00
  0x0F, 0x30,                   ///< wrmsr
  0xC3                          ///< ret
};

STATIC
EFI_STATUS
PatchAppleXcpmForceBoost (
  IN OUT PATCHER_CONTEXT    *Patcher,
  IN     UINT32             KernelVersion
  )
{
  UINT8   *Start;
  UINT8   *Last;
  UINT8   *Current;

  ASSERT (Patcher != NULL);
  if (!Patcher) {
    return EFI_INVALID_PARAMETER;
  }

  if (!OcMatchDarwinVersion (KernelVersion, KERNEL_VERSION_MOUNTAIN_LION_MIN, 0)) {
    DEBUG ((DEBUG_INFO, "OCAK: Skipping XcpmForceBoost on %u\n", KernelVersion));
    return EFI_SUCCESS;
  }

  Start   = (UINT8 *) MachoGetMachHeader64 (&Patcher->MachContext);
  Last    = Start + MachoGetFileSize (&Patcher->MachContext) - EFI_PAGE_SIZE*2;
  Start  += EFI_PAGE_SIZE;
  Current = Start;

  while (Current < Last) {
    if (Current[0] == mPerfCtrlFind1[0]
      && Current[1] == mPerfCtrlFind1[1]
      && Current[2] == mPerfCtrlFind1[2]
      && Current[3] == mPerfCtrlFind1[3]) {
      if (CompareMem (&Current[4], &mPerfCtrlFind1[4], sizeof (mPerfCtrlFind1) - 4) == 0
        || CompareMem (&Current[4], &mPerfCtrlFind2[4], sizeof (mPerfCtrlFind2) - 4) == 0) {
        break;
      }
    }

    ++Current;
  }

  if (Current == Last) {
    DEBUG ((DEBUG_WARN, "OCAK: Failed to locate MSR_IA32_PERF_CONTROL write\n"));
    return EFI_NOT_FOUND;
  }

  Start    = Current - EFI_PAGE_SIZE;
  Current -= 4;

  while (Current >= Start) {
    if (Current[0] == 0x55
      && Current[1] == 0x48
      && Current[2] == 0x89
      && Current[3] == 0xE5) {
      break;
    }

    --Current;
  }

  if (Current < Start) {
    DEBUG ((DEBUG_WARN, "OCAK: Failed to locate MSR_IA32_PERF_CONTROL prologue\n"));
    return EFI_NOT_FOUND;
  }

  DEBUG ((DEBUG_INFO, "OCAK: Patch write max to MSR_IA32_PERF_CONTROL\n"));
  CopyMem (Current, mPerfCtrlMax, sizeof (mPerfCtrlMax));
  return EFI_SUCCESS;
}

STATIC
UINT8
mRemoveUsbLimitV1Find[] = {
  0xff, 0xff, 0x10
};

STATIC
UINT8
mRemoveUsbLimitV1Replace[] = {
  0xff, 0xff, 0x40
};

STATIC
PATCHER_GENERIC_PATCH
mRemoveUsbLimitV1Patch = {
  .Comment     = DEBUG_POINTER ("RemoveUsbLimitV1"),
  .Base        = "__ZN15AppleUSBXHCIPCI11createPortsEv",
  .Find        = mRemoveUsbLimitV1Find,
  .Mask        = NULL,
  .Replace     = mRemoveUsbLimitV1Replace,
  .ReplaceMask = NULL,
  .Size        = sizeof (mRemoveUsbLimitV1Replace),
  .Count       = 1,
  .Skip        = 0,
  .Limit       = 8192
};

STATIC
UINT8
mRemoveUsbLimitV2Find[] = {
  0x0f, 0x0f, 0x83
};

STATIC
UINT8
mRemoveUsbLimitV2Replace[] = {
  0x40, 0x0f, 0x83
};

STATIC
PATCHER_GENERIC_PATCH
mRemoveUsbLimitV2Patch = {
  .Comment     = DEBUG_POINTER ("RemoveUsbLimitV2"),
  .Base        = "__ZN12AppleUSBXHCI11createPortsEv",
  .Find        = mRemoveUsbLimitV2Find,
  .Mask        = NULL,
  .Replace     = mRemoveUsbLimitV2Replace,
  .ReplaceMask = NULL,
  .Size        = sizeof (mRemoveUsbLimitV2Replace),
  .Count       = 1,
  .Skip        = 0,
  .Limit       = 4096
};

STATIC
UINT8
mRemoveUsbLimitIoP1Find[] = {
  0x0f, 0x0f, 0x87
};

STATIC
UINT8
mRemoveUsbLimitIoP1Replace[] = {
  0x40, 0x0f, 0x87
};

STATIC
PATCHER_GENERIC_PATCH
mRemoveUsbLimitIoP1Patch = {
  .Comment     = DEBUG_POINTER ("RemoveUsbLimitIoP1"),
  .Base        = "__ZN16AppleUSBHostPort15setPortLocationEj",
  .Find        = mRemoveUsbLimitIoP1Find,
  .Mask        = NULL,
  .Replace     = mRemoveUsbLimitIoP1Replace,
  .ReplaceMask = NULL,
  .Size        = sizeof (mRemoveUsbLimitIoP1Replace),
  .Count       = 1,
  .Skip        = 0,
  .Limit       = 4096
};

STATIC
EFI_STATUS
PatchUsbXhciPortLimit1 (
  IN OUT PATCHER_CONTEXT    *Patcher,
  IN     UINT32             KernelVersion
  )
{
  EFI_STATUS  Status;

  //
  // On 10.14.4 and newer IOUSBHostFamily also needs limit removal.
  // Thanks to ydeng discovering this.
  //
  if (!OcMatchDarwinVersion (KernelVersion, KERNEL_VERSION(18, 5, 0), 0)) {
    DEBUG ((DEBUG_INFO, "OCAK: Skipping port patch IOUSBHostFamily on %u\n", KernelVersion));
    return EFI_SUCCESS;
  }

  if (Patcher == NULL) {
    return EFI_NOT_FOUND;
  }

  Status = PatcherApplyGenericPatch (Patcher, &mRemoveUsbLimitIoP1Patch);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCAK: Failed to apply port patch com.apple.iokit.IOUSBHostFamily - %r\n", Status));
  } else {
    DEBUG ((DEBUG_INFO, "OCAK: Patch success port com.apple.iokit.IOUSBHostFamily\n"));
  }

  return Status;
}

STATIC
EFI_STATUS
PatchUsbXhciPortLimit2 (
  IN OUT PATCHER_CONTEXT    *Patcher,
  IN     UINT32             KernelVersion
  )
{
  EFI_STATUS  Status;

  if (!OcMatchDarwinVersion (KernelVersion, KERNEL_VERSION_HIGH_SIERRA_MIN, 0)) {
    DEBUG ((DEBUG_INFO, "OCAK: Skipping legacy port patch AppleUSBXHCIPCI on %u\n", KernelVersion));
    return EFI_SUCCESS;
  }

  if (Patcher == NULL) {
    return EFI_NOT_FOUND;
  }

  //
  // TODO: Implement some locationID hack in IOUSBHostFamily.
  // The location ID is a 32 bit number which is unique among all USB devices in the system,
  // and which will not change on a system reboot unless the topology of the bus itself changes.
  // See AppleUSBHostPort::setPortLocation():
  // locationId = getLocationId();
  // if (!(locationId & 0xF)) {
  //   int32_t shift = 20;
  //   while (locationId & (0xF << shift)) {
  //     shift -= 4;
  //     if (Shift < 0) { setLocationId(locationId); return; }
  //   }
  //   setLocationId(locationId | ((portNumber & 0xF) << shift));
  // }
  // The value (e.g. 0x14320000) is represented as follows: 0xAABCDEFG
  // AA  — Ctrl number 8 bits (e.g. 0x14, aka XHCI)
  // B   - Port number 4 bits (e.g. 0x3, aka SS03)
  // C~F - Bus number  4 bits (e.g. 0x2, aka IOUSBHostHIDDevice)
  //
  // C~F are filled as many times as many USB Hubs are there on the port.
  //
  Status = PatcherApplyGenericPatch (Patcher, &mRemoveUsbLimitV2Patch);
  if (!EFI_ERROR (Status)) {
    //
    // We do not need to patch com.apple.driver.usb.AppleUSBXHCI if this patch was successful.
    // Only legacy systems require com.apple.driver.usb.AppleUSBXHCI to be patched.
    //
    DEBUG ((DEBUG_INFO, "OCAK: Patch success com.apple.driver.usb.AppleUSBXHCI\n"));
  } else {
    DEBUG ((DEBUG_INFO, "OCAK: Failed to apply patch com.apple.driver.usb.AppleUSBXHCI - %r\n", Status));
  }

  //
  // TODO: Check when the patch changed actually.
  //
  if (EFI_ERROR (Status)
    && OcMatchDarwinVersion (KernelVersion, KERNEL_VERSION_HIGH_SIERRA_MIN, KERNEL_VERSION_HIGH_SIERRA_MAX)) {
    DEBUG ((DEBUG_INFO, "OCAK: Assuming success for AppleUSBXHCI on %u\n", KernelVersion));
    return EFI_SUCCESS;
  }

  return Status;
}

STATIC
EFI_STATUS
PatchUsbXhciPortLimit3 (
  IN OUT PATCHER_CONTEXT    *Patcher,
  IN     UINT32             KernelVersion
  )
{
  EFI_STATUS  Status;

  if (!OcMatchDarwinVersion (KernelVersion, KERNEL_VERSION_EL_CAPITAN_MIN, KERNEL_VERSION_HIGH_SIERRA_MAX)) {
    DEBUG ((DEBUG_INFO, "OCAK: Skipping legacy port patch AppleUSBXHCIPCI on %u\n", KernelVersion));
    return EFI_SUCCESS;
  }

  if (Patcher == NULL) {
    return EFI_NOT_FOUND;
  }

  //
  // If we are here, we are on legacy 10.13 or below, try the oldest patch.
  //
  Status = PatcherApplyGenericPatch (Patcher, &mRemoveUsbLimitV1Patch);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCAK: Failed to apply legacy port patch AppleUSBXHCIPCI - %r\n", Status));
  } else {
    DEBUG ((DEBUG_INFO, "OCAK: Patch success apply legacy port AppleUSBXHCIPCI\n"));
  }

  //
  // TODO: Check when the patch changed actually.
  //
  if (EFI_ERROR (Status)
    && OcMatchDarwinVersion (KernelVersion, KERNEL_VERSION_HIGH_SIERRA_MIN, KERNEL_VERSION_HIGH_SIERRA_MAX)) {
    DEBUG ((DEBUG_INFO, "OCAK: Assuming success for legacy port AppleUSBXHCIPCI on %u\n", KernelVersion));
    return EFI_SUCCESS;
  }

  return Status;
}

STATIC
UINT8
mIOAHCIBlockStoragePatchV1Find[] = {
  0x41, 0x50, 0x50, 0x4C, 0x45, 0x20, 0x53, 0x53, 0x44, 0x00
};

STATIC
UINT8
mIOAHCIBlockStoragePatchV1Replace[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

STATIC
PATCHER_GENERIC_PATCH
mIOAHCIBlockStoragePatchV1 = {
  .Comment     = DEBUG_POINTER ("IOAHCIBlockStorageV1"),
  .Base        = NULL,
  .Find        = mIOAHCIBlockStoragePatchV1Find,
  .Mask        = NULL,
  .Replace     = mIOAHCIBlockStoragePatchV1Replace,
  .ReplaceMask = NULL,
  .Size        = sizeof (mIOAHCIBlockStoragePatchV1Find),
  .Count       = 1,
  .Skip        = 0
};

STATIC
UINT8
mIOAHCIBlockStoragePatchV2Find[] = {
  0x41, 0x50, 0x50, 0x4C, 0x45, 0x00
};

STATIC
UINT8
mIOAHCIBlockStoragePatchV2Replace[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

STATIC
PATCHER_GENERIC_PATCH
mIOAHCIBlockStoragePatchV2 = {
  .Comment     = DEBUG_POINTER ("IOAHCIBlockStorageV2"),
  .Base        = NULL,
  .Find        = mIOAHCIBlockStoragePatchV2Find,
  .Mask        = NULL,
  .Replace     = mIOAHCIBlockStoragePatchV2Replace,
  .ReplaceMask = NULL,
  .Size        = sizeof (mIOAHCIBlockStoragePatchV2Find),
  .Count       = 1,
  .Skip        = 0
};

STATIC
EFI_STATUS
PatchThirdPartyDriveSupport (
  IN OUT PATCHER_CONTEXT    *Patcher,
  IN     UINT32             KernelVersion
  )
{
  EFI_STATUS       Status;

  if (Patcher == NULL) {
    return EFI_NOT_FOUND;
  }

  Status = PatcherApplyGenericPatch (Patcher, &mIOAHCIBlockStoragePatchV1);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCAK: Failed to apply patch com.apple.iokit.IOAHCIBlockStorage V1 - %r\n", Status));
  } else {
    DEBUG ((DEBUG_INFO, "OCAK: Patch success com.apple.iokit.IOAHCIBlockStorage V1\n"));
  }

  if (OcMatchDarwinVersion (KernelVersion, KERNEL_VERSION_CATALINA_MIN, 0)) {
    Status = PatcherApplyGenericPatch (Patcher, &mIOAHCIBlockStoragePatchV2);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "OCAK: Failed to apply patch com.apple.iokit.IOAHCIBlockStorage V2 - %r\n", Status));
    } else {
      DEBUG ((DEBUG_INFO, "OCAK: Patch success com.apple.iokit.IOAHCIBlockStorage V2\n"));
    }
  } else {
    DEBUG ((DEBUG_INFO, "OCAK: Skipping IOAHCIBlockStorage V2 on %u\n", KernelVersion));
  }

  //
  // This started to be required on 10.6.7 or so.
  // We cannot trust which minor SnowLeo version is this, just let it pass.
  //
  if (EFI_ERROR (Status)
    && OcMatchDarwinVersion (KernelVersion, KERNEL_VERSION_SNOW_LEOPARD_MIN, KERNEL_VERSION_SNOW_LEOPARD_MAX)) {
    DEBUG ((DEBUG_INFO, "OCAK: Assuming success for IOAHCIBlockStorage on %u\n", KernelVersion));
    return EFI_SUCCESS;
  }

  return Status;
}

STATIC
UINT8
mIOAHCIPortPatchFind[] = {
  0x45, 0x78, 0x74, 0x65, 0x72, 0x6E, 0x61, 0x6C
};

STATIC
UINT8
mIOAHCIPortPatchReplace[] = {
  0x49, 0x6E, 0x74, 0x65, 0x72, 0x6E, 0x61, 0x6C
};

STATIC
PATCHER_GENERIC_PATCH
mIOAHCIPortPatch = {
  .Comment      = DEBUG_POINTER ("IOAHCIPort"),
  .Base         = NULL,
  .Find         = mIOAHCIPortPatchFind,
  .Mask         = NULL,
  .Replace      = mIOAHCIPortPatchReplace,
  .ReplaceMask  = NULL,
  .Size         = sizeof (mIOAHCIPortPatchFind),
  .Count        = 1,
  .Skip         = 0
};

STATIC
EFI_STATUS
PatchForceInternalDiskIcons (
  IN OUT PATCHER_CONTEXT    *Patcher,
  IN     UINT32             KernelVersion
  )
{
  EFI_STATUS          Status;

  if (Patcher == NULL) {
    return EFI_NOT_FOUND;
  }

  Status = PatcherApplyGenericPatch (Patcher, &mIOAHCIPortPatch);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCAK: Failed to apply patch com.apple.driver.AppleAHCIPort - %r\n", Status));
  } else {
    DEBUG ((DEBUG_INFO, "OCAK: Patch success com.apple.driver.AppleAHCIPort\n"));
  }

  return Status;
}

STATIC
UINT8
mAppleIoMapperPatchFind[] = {
  0x44, 0x4D, 0x41, 0x52, 0x00 // DMAR\0
};

STATIC
UINT8
mAppleIoMapperPatchReplace[] = {
  0x52, 0x41, 0x4D, 0x44, 0x00 // RAMD\0
};

STATIC
PATCHER_GENERIC_PATCH
mAppleIoMapperPatch = {
  .Comment     = DEBUG_POINTER ("AppleIoMapper"),
  .Base        = NULL,
  .Find        = mAppleIoMapperPatchFind,
  .Mask        = NULL,
  .Replace     = mAppleIoMapperPatchReplace,
  .ReplaceMask = NULL,
  .Size        = sizeof (mAppleIoMapperPatchFind),
  .Count       = 1,
  .Skip        = 0
};

STATIC
EFI_STATUS
PatchAppleIoMapperSupport (
  IN OUT PATCHER_CONTEXT    *Patcher,
  IN     UINT32             KernelVersion
  )
{
  EFI_STATUS          Status;

  if (Patcher == NULL) {
    return EFI_NOT_FOUND;
  }

  if (!OcMatchDarwinVersion (KernelVersion, KERNEL_VERSION_MOUNTAIN_LION_MIN, 0)) {
    DEBUG ((DEBUG_INFO, "OCAK: Skipping AppleIoMapper patch on %u\n", KernelVersion));
    return EFI_SUCCESS;
  }

  Status = PatcherApplyGenericPatch (Patcher, &mAppleIoMapperPatch);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCAK: Failed to apply patch com.apple.iokit.IOPCIFamily AppleIoMapper - %r\n", Status));
  } else {
    DEBUG ((DEBUG_INFO, "OCAK: Patch success com.apple.iokit.IOPCIFamily AppleIoMapper\n"));
  }

  return Status;
}

STATIC
UINT8
mAppleDummyCpuPmPatchReplace[] = {
  0xB8, 0x01, 0x00, 0x00, 0x00, 0xC3 // mov eax, 1 ; ret
};

STATIC
PATCHER_GENERIC_PATCH
mAppleDummyCpuPmPatch = {
  .Comment     = DEBUG_POINTER ("DummyCpuPm"),
  .Base        = "__ZN28AppleIntelCPUPowerManagement5startEP9IOService",
  .Find        = NULL,
  .Mask        = NULL,
  .Replace     = mAppleDummyCpuPmPatchReplace,
  .ReplaceMask = NULL,
  .Size        = sizeof (mAppleDummyCpuPmPatchReplace),
  .Count       = 1,
  .Skip        = 0
};

STATIC
EFI_STATUS
PatchDummyPowerManagement (
  IN OUT PATCHER_CONTEXT    *Patcher,
  IN     UINT32             KernelVersion
  )
{
  EFI_STATUS          Status;

  if (Patcher == NULL) {
    return EFI_NOT_FOUND;
  }

  Status = PatcherApplyGenericPatch (Patcher, &mAppleDummyCpuPmPatch);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCAK: Failed to apply patch dummy AppleIntelCPUPowerManagement - %r\n", Status));
  } else {
    DEBUG ((DEBUG_INFO, "OCAK: Patch success dummy AppleIntelCPUPowerManagement\n"));
  }

  return Status;
}

STATIC
UINT8
mIncreasePciBarSizePatchFind[] = {
  0x00, 0x00, 0x00, 0x40
};

STATIC
UINT8
mIncreasePciBarSizePatchReplace[] = {
  0x00, 0x00, 0x00, 0x80
};

STATIC
PATCHER_GENERIC_PATCH
mIncreasePciBarSizePatch = {
  .Comment     = DEBUG_POINTER ("IncreasePciBarSize"),
  .Base        = "__ZN17IOPCIConfigurator24probeBaseAddressRegisterEP16IOPCIConfigEntryjj",
  .Find        = mIncreasePciBarSizePatchFind,
  .Mask        = NULL,
  .Replace     = mIncreasePciBarSizePatchReplace,
  .ReplaceMask = NULL,
  .Size        = sizeof (mIncreasePciBarSizePatchFind),
  .Count       = 1,
  .Skip        = 0,
  .Limit       = EFI_PAGE_SIZE
};

STATIC
UINT8
mIncreasePciBarSizePatchLegacyFind[] = {
  0x01, 0x00, 0x00, 0x40
};

STATIC
UINT8
mIncreasePciBarSizePatchLegacyReplace[] = {
  0x01, 0x00, 0x00, 0x80
};

STATIC
PATCHER_GENERIC_PATCH
mIncreasePciBarSizeLegacyPatch = {
  .Comment     = DEBUG_POINTER ("IncreasePciBarSizeLegacy"),
  .Base        = "__ZN17IOPCIConfigurator24probeBaseAddressRegisterEP16IOPCIConfigEntryjj",
  .Find        = mIncreasePciBarSizePatchLegacyFind,
  .Mask        = NULL,
  .Replace     = mIncreasePciBarSizePatchLegacyReplace,
  .ReplaceMask = NULL,
  .Size        = sizeof (mIncreasePciBarSizePatchLegacyFind),
  .Count       = 1,
  .Skip        = 0,
  .Limit       = EFI_PAGE_SIZE
};

STATIC
EFI_STATUS
PatchIncreasePciBarSize (
  IN OUT PATCHER_CONTEXT    *Patcher,
  IN     UINT32             KernelVersion
  )
{
  EFI_STATUS          Status;

  if (Patcher == NULL) {
    return EFI_NOT_FOUND;
  }

  if (!OcMatchDarwinVersion (KernelVersion, KERNEL_VERSION_YOSEMITE_MIN, 0)) {
    DEBUG ((DEBUG_INFO, "OCAK: Skipping com.apple.iokit.IOPCIFamily IncreasePciBarSize on %u\n", KernelVersion));
    return EFI_SUCCESS;
  }

  Status = PatcherApplyGenericPatch (Patcher, &mIncreasePciBarSizePatch);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCAK: Failed to apply patch com.apple.iokit.IOPCIFamily IncreasePciBarSize - %r\n", Status));
    Status = PatcherApplyGenericPatch (Patcher, &mIncreasePciBarSizeLegacyPatch);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "OCAK: Failed to apply legacy patch com.apple.iokit.IOPCIFamily IncreasePciBarSize - %r\n", Status));
    } else {
      DEBUG ((DEBUG_INFO, "OCAK: Patch success legacy com.apple.iokit.IOPCIFamily IncreasePciBarSize\n"));
    }
  } else {
    DEBUG ((DEBUG_INFO, "OCAK: Patch success com.apple.iokit.IOPCIFamily IncreasePciBarSize\n"));
  }

  return Status;
}


STATIC
CONST UINT8
mKernelCpuIdFindRelNew[] = {
  0xB9, 0x8B, 0x00, 0x00, 0x00, ///< mov ecx, 8Bh
  0x31, 0xC0,                   ///< xor eax, eax
  0x31, 0xD2,                   ///< xor edx, edx
  0x0F, 0x30,                   ///< wrmsr
  0xB8, 0x01, 0x00, 0x00, 0x00, ///< mov eax, 1
  0x31, 0xDB,                   ///< xor ebx, ebx
  0x31, 0xC9,                   ///< xor ecx, ecx
  0x31, 0xD2,                   ///< xor edx, edx
  0x0F, 0xA2                    ///< cpuid
};

STATIC
CONST UINT8
mKernelCpuIdFindRelOld[] = {
  0xB9, 0x8B, 0x00, 0x00, 0x00, ///< mov ecx, 8Bh
  0x31, 0xD2,                   ///< xor edx, edx
  0x0F, 0x30,                   ///< wrmsr
  0xB8, 0x01, 0x00, 0x00, 0x00, ///< mov eax, 1
  0x31, 0xDB,                   ///< xor ebx, ebx
  0x31, 0xC9,                   ///< xor ecx, ecx
  0x31, 0xD2,                   ///< xor edx, edx
  0x0F, 0xA2                    ///< cpuid
};

STATIC
CONST UINT8
mKernelCpuidFindMcRel[] = {
  0xB9, 0x8B, 0x00, 0x00, 0x00, 0x0F, 0x32
};

/**
  cpu->cpuid_signature           = 0x11111111;
  cpu->cpuid_stepping            = 0x22;
  cpu->cpuid_model               = 0x33;
  cpu->cpuid_family              = 0x44;
  cpu->cpuid_type                = 0x55555555;
  cpu->cpuid_extmodel            = 0x66;
  cpu->cpuid_extfamily           = 0x77;
  cpu->cpuid_features            = 0x8888888888888888;
  cpu->cpuid_logical_per_package = 0x99999999;
  cpu->cpuid_cpufamily           = 0xAAAAAAAA;
  return 0xAAAAAAAA;
**/


STATIC
CONST UINT8
mKernelCpuidReplaceDbg[] = {
  0xC7, 0x47, 0x68, 0x11, 0x11, 0x11, 0x11,                   ///< mov dword ptr [rdi+68h], 11111111h
  0xC6, 0x47, 0x50, 0x22,                                     ///< mov byte ptr [rdi+50h], 22h
  0x48, 0xB8, 0x55, 0x55, 0x55, 0x55, 0x44, 0x33, 0x66, 0x77, ///< mov rax, 7766334455555555h
  0x48, 0x89, 0x47, 0x48,                                     ///< mov [rdi+48h], rax
  0x48, 0xB8, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, ///< mov rax, 8888888888888888h
  0x48, 0x89, 0x47, 0x58,                                     ///< mov [rdi+58h], rax
  0xC7, 0x87, 0xCC, 0x00, 0x00, 0x00, 0x99, 0x99, 0x99, 0x99, ///< mov dword ptr [rdi+0CCh], 99999999h
  0xC7, 0x87, 0x88, 0x01, 0x00, 0x00, 0xAA, 0xAA, 0xAA, 0xAA, ///< mov dword ptr [rdi+188h], 0AAAAAAAAh
  0xB8, 0xAA, 0xAA, 0xAA, 0xAA,                               ///< mov eax, 0AAAAAAAAh
  0xC3                                                        ///< retn
};

#pragma pack(push, 1)

typedef struct {
  UINT8   Code1[3];
  UINT32  Signature;
  UINT8   Code2[3];
  UINT8   Stepping;
  UINT8   Code3[2];
  UINT32  Type;
  UINT8   Family;
  UINT8   Model;
  UINT8   ExtModel;
  UINT8   ExtFamily;
  UINT8   Code4[6];
  UINT64  Features;
  UINT8   Code5[10];
  UINT32  LogicalPerPkg;
  UINT8   Code6[6];
  UINT32  AppleFamily1;
  UINT8   Code7;
  UINT32  AppleFamily2;
  UINT8   Code8;
} INTERNAL_CPUID_FN_PATCH;

STATIC_ASSERT (
  sizeof (INTERNAL_CPUID_FN_PATCH) == sizeof (mKernelCpuidReplaceDbg),
  "Check your CPUID patch layout"
  );

typedef struct {
  UINT8   EaxCmd;
  UINT32  EaxVal;
  UINT8   EbxCmd;
  UINT32  EbxVal;
  UINT8   EcxCmd;
  UINT32  EcxVal;
  UINT8   EdxCmd;
  UINT32  EdxVal;
} INTERNAL_CPUID_PATCH;

typedef struct {
  UINT8   EdxCmd;
  UINT32  EdxVal;
} INTERNAL_MICROCODE_PATCH;

#pragma pack(pop)

STATIC
EFI_STATUS
PatchKernelCpuIdLegacy (
  IN OUT PATCHER_CONTEXT  *Patcher,
  IN     OC_CPU_INFO      *CpuInfo,
  IN     UINT32           *Data,
  IN     UINT32           *DataMask,
  IN     UINT8            *Record
  )
{
  UINT8                 *StartPointer;
  UINT8                 *EndPointer;
  UINT8                 *Location;
  UINT8                 *LocationEnd;
  UINT32                Signature[3];
  UINT32                Index;
  UINT32                MaxExt;
  INT32                 Delta;
  INTERNAL_CPUID_PATCH  Patch;

  STATIC CONST UINT8 mKernelCpuidFindLegacyStart[8] = {
    0xBA, 0x04, 0x00, 0x00, 0x00,  ///< mov edx, 4
    0x48, 0x8D, 0x35               ///< lea rsi from lea rsi, byte_FFFFFF80006C5900
  };

  for (Index = 0; Index < EFI_PAGE_SIZE; ++Index, ++Record) {
    if (Record[0] == mKernelCpuidFindLegacyStart[0]
      && Record[1] == mKernelCpuidFindLegacyStart[1]
      && Record[2] == mKernelCpuidFindLegacyStart[2]
      && Record[3] == mKernelCpuidFindLegacyStart[3]
      && Record[4] == mKernelCpuidFindLegacyStart[4]
      && Record[5] == mKernelCpuidFindLegacyStart[5]
      && Record[6] == mKernelCpuidFindLegacyStart[6]
      && Record[7] == mKernelCpuidFindLegacyStart[7]) {
      break;
    }
  }

  if (Index >= EFI_PAGE_SIZE) {
    return EFI_NOT_FOUND;
  }

  StartPointer = Record + sizeof (mKernelCpuidFindLegacyStart) + sizeof (UINT32);

  STATIC CONST UINT8 mKernelCpuidFindLegacyEnd1[6] = {
    0x00,                         ///< 0 from mov cs:byte_FFFFFF80006C590C, 0
    0xB8, 0x00, 0x00, 0x00, 0x80, ///< mov eax, 80000000h
  };

  for (; Index < EFI_PAGE_SIZE; ++Index, ++Record) {
    if (Record[0] == mKernelCpuidFindLegacyEnd1[0]
      && Record[1] == mKernelCpuidFindLegacyEnd1[1]
      && Record[2] == mKernelCpuidFindLegacyEnd1[2]
      && Record[3] == mKernelCpuidFindLegacyEnd1[3]
      && Record[4] == mKernelCpuidFindLegacyEnd1[4]
      && Record[5] == mKernelCpuidFindLegacyEnd1[5]) {
      break;
    }
  }

  if (Index >= EFI_PAGE_SIZE) {
    return EFI_NOT_FOUND;
  }

  STATIC CONST UINT8 mKernelCpuidFindLegacyEnd2[3] = {
    0x0F, 0xA2,                   ///< cpuid
    0x89                          ///< mov prefix from mov [rbp+var_40], eax
  };

  for (; Index < EFI_PAGE_SIZE; ++Index, ++Record) {
    if (Record[0] == mKernelCpuidFindLegacyEnd2[0]
      && Record[1] == mKernelCpuidFindLegacyEnd2[1]
      && Record[2] == mKernelCpuidFindLegacyEnd2[2]) {
      break;
    }
  }

  if (Index >= EFI_PAGE_SIZE) {
    return EFI_NOT_FOUND;
  }

  EndPointer = Record - 3;

  STATIC CONST UINT8 mKernelCpuidFindLegacyLoc1[5] = {
    0xB8, 0x01, 0x00, 0x00, 0x00, ///< mov eax, 1
  };

  for (; Index < EFI_PAGE_SIZE; ++Index, ++Record) {
    if (Record[0] == mKernelCpuidFindLegacyLoc1[0]
      && Record[1] == mKernelCpuidFindLegacyLoc1[1]
      && Record[2] == mKernelCpuidFindLegacyLoc1[2]
      && Record[3] == mKernelCpuidFindLegacyLoc1[3]
      && Record[4] == mKernelCpuidFindLegacyLoc1[4]) {
      break;
    }
  }

  if (Index >= EFI_PAGE_SIZE) {
    return EFI_NOT_FOUND;
  }

  Location = Record;

  STATIC CONST UINT8 mKernelCpuidFindLegacyLoc2[3] = {
    0x0F, 0xA2,                   ///< cpuid
    0x89                          ///< mov prefix from mov [rbp+var_40], eax
  };

  for (; Index < EFI_PAGE_SIZE; ++Index, ++Record) {
    if (Record[0] == mKernelCpuidFindLegacyLoc2[0]
      && Record[1] == mKernelCpuidFindLegacyLoc2[1]
      && Record[2] == mKernelCpuidFindLegacyLoc2[2]) {
      break;
    }
  }

  if (Index >= EFI_PAGE_SIZE) {
    return EFI_NOT_FOUND;
  }

  LocationEnd = Record + 2;

  //
  // Free 2+ more bytes in the end by assigning EAX directly.
  //
  AsmCpuid (0x80000000, &MaxExt, NULL, NULL, NULL);
  EndPointer[0] = 0xB8;
  CopyMem (&EndPointer[1], &MaxExt, sizeof (MaxExt));

  //
  // Short-write CPU signature at RSI save space.
  //
  AsmCpuid (0, NULL, &Signature[0], &Signature[2], &Signature[1]);
  for (Index = 0; Index < 3; ++Index) {
    //
    // mov eax, <signature>
    //
    *StartPointer++ = 0xB8;
    CopyMem (StartPointer, &Signature[Index], sizeof (Signature[0]));
    StartPointer += sizeof (Signature[0]);
    //
    // mov [rsi], eax
    //
    *StartPointer++ = 0x89;
    *StartPointer++ = 0x06;
    if (Index < 2) {
      //
      // add rsi, 4
      //
      *StartPointer++ = 0x48;
      *StartPointer++ = 0x83;
      *StartPointer++ = 0xC6;
      *StartPointer++ = 0x04;
    }
  }

  //
  // Ensure that we still have room, which is within 2-byte jmp (127)
  // and has at least 2-byte jmp and patch + 5-byte jmp area.
  // Should have around 25 bytes here.
  //
  if (StartPointer >= EndPointer
    || EndPointer - StartPointer > 128
    || (UINTN) (EndPointer - StartPointer) < sizeof (INTERNAL_CPUID_PATCH) + 7) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Short jmp to EndPointer
  //
  StartPointer[0] = 0xEB;
  StartPointer[1] = (UINT8) (EndPointer - StartPointer - 2);
  StartPointer += 2;

  //
  // Workaround incorrect OSXSAVE handling, see below.
  //
  if (CpuInfo->CpuidVerEcx.Bits.XSAVE != 0
    && CpuInfo->CpuidVerEcx.Bits.OSXSAVE == 0
    && CpuInfo->CpuidVerEcx.Bits.AVX != 0) {
    CpuInfo->CpuidVerEcx.Bits.OSXSAVE = 1;
  }

  //
  // Jump from location to patch area.
  //
  Delta = (INT32) (StartPointer - (Location + 5));
  *Location++ = 0xE9;
  CopyMem (Location, &Delta, sizeof (Delta));
  Location   += sizeof (Delta);
  while (Location < LocationEnd) {
    *Location++ = 0x90;
  }

  //
  // Write virtualised CPUID.
  //
  Patch.EaxCmd = 0xB8;
  Patch.EaxVal = (Data[0] & DataMask[0]) | (CpuInfo->CpuidVerEax.Uint32 & ~DataMask[0]);
  Patch.EbxCmd = 0xBB;
  Patch.EbxVal = (Data[1] & DataMask[1]) | (CpuInfo->CpuidVerEbx.Uint32 & ~DataMask[1]);
  Patch.EcxCmd = 0xB9;
  Patch.EcxVal = (Data[2] & DataMask[2]) | (CpuInfo->CpuidVerEcx.Uint32 & ~DataMask[2]);
  Patch.EdxCmd = 0xBA;
  Patch.EdxVal = (Data[3] & DataMask[3]) | (CpuInfo->CpuidVerEdx.Uint32 & ~DataMask[3]);
  CopyMem (StartPointer, &Patch, sizeof (Patch));
  StartPointer += sizeof (Patch);

  //
  // Jump to the end of location (5-byte mov + 2-byte cpuid).
  //
  Delta = (INT32) (Location - (StartPointer + 5));
  *StartPointer++ = 0xE9;
  CopyMem (StartPointer, &Delta, sizeof (Delta));

  return EFI_SUCCESS;
}

EFI_STATUS
PatchKernelCpuId (
  IN OUT PATCHER_CONTEXT  *Patcher,
  IN     OC_CPU_INFO      *CpuInfo,
  IN     UINT32           *Data,
  IN     UINT32           *DataMask
  )
{
  EFI_STATUS                Status;
  UINT8                     *CpuidSetInfo;
  UINT8                     *Record;
  UINT8                     *Last;
  UINT32                    Index;
  UINT32                    FoundSize;
  INTERNAL_CPUID_PATCH      *CpuidPatch;
  INTERNAL_MICROCODE_PATCH  *McPatch;
  INTERNAL_CPUID_FN_PATCH   *FnPatch;
  CPUID_VERSION_INFO_EAX    Eax;
  CPUID_VERSION_INFO_EBX    Ebx;
  CPUID_VERSION_INFO_ECX    Ecx;
  CPUID_VERSION_INFO_EDX    Edx;
  BOOLEAN                   FoundReleaseKernel;

  ASSERT (Patcher != NULL);

  STATIC_ASSERT (
    sizeof (mKernelCpuIdFindRelNew) > sizeof (mKernelCpuIdFindRelOld),
    "Kernel CPUID patch seems wrong"
    );

  ASSERT (mKernelCpuIdFindRelNew[0] == mKernelCpuIdFindRelOld[0]
    && mKernelCpuIdFindRelNew[1] == mKernelCpuIdFindRelOld[1]
    && mKernelCpuIdFindRelNew[2] == mKernelCpuIdFindRelOld[2]
    && mKernelCpuIdFindRelNew[3] == mKernelCpuIdFindRelOld[3]
    );

  Last = ((UINT8 *) MachoGetMachHeader64 (&Patcher->MachContext)
    + MachoGetFileSize (&Patcher->MachContext) - EFI_PAGE_SIZE*2 - sizeof (mKernelCpuIdFindRelNew));

  Status = PatcherGetSymbolAddress (Patcher, "_cpuid_set_info", (UINT8 **) &CpuidSetInfo);
  if (EFI_ERROR (Status) || CpuidSetInfo >= Last) {
    DEBUG ((DEBUG_WARN, "OCAK: Failed to locate _cpuid_set_info (%p) - %r\n", CpuidSetInfo, Status));
    return EFI_NOT_FOUND;
  }

  Record = CpuidSetInfo;
  FoundSize = 0;

  for (Index = 0; Index < EFI_PAGE_SIZE; ++Index, ++Record) {
    if (Record[0] == mKernelCpuIdFindRelNew[0]
      && Record[1] == mKernelCpuIdFindRelNew[1]
      && Record[2] == mKernelCpuIdFindRelNew[2]
      && Record[3] == mKernelCpuIdFindRelNew[3]) {

      if (CompareMem (Record, mKernelCpuIdFindRelNew, sizeof (mKernelCpuIdFindRelNew)) == 0) {
        FoundSize = sizeof (mKernelCpuIdFindRelNew);
        break;
      } else if (CompareMem (Record, mKernelCpuIdFindRelOld, sizeof (mKernelCpuIdFindRelOld)) == 0) {
        FoundSize = sizeof (mKernelCpuIdFindRelOld);
        break;
      }
    }
  }

  FoundReleaseKernel = FoundSize > 0;

  if (!FoundReleaseKernel) {
    Status = PatchKernelCpuIdLegacy (Patcher, CpuInfo, Data, DataMask, CpuidSetInfo);
    if (!EFI_ERROR (Status)) {
      return EFI_SUCCESS;
    }
  }

  //
  // When patching the release kernel we do not allow reevaluating CPUID information,
  // which is used to report OSXSAVE availability. This causes issues with some programs,
  // like Docker using Hypervisor.framework, which rely on sysctl to track CPU feature.
  //
  // To workaround this we make sure to always report OSXSAVE bit when it is available
  // regardless of the reevaluation performed by init_fpu in XNU.
  //
  // REF: https://github.com/acidanthera/bugtracker/issues/1035
  //
  if (FoundReleaseKernel
    && CpuInfo->CpuidVerEcx.Bits.XSAVE != 0
    && CpuInfo->CpuidVerEcx.Bits.OSXSAVE == 0
    && CpuInfo->CpuidVerEcx.Bits.AVX != 0) {
    CpuInfo->CpuidVerEcx.Bits.OSXSAVE = 1;
  }

  Eax.Uint32 = (Data[0] & DataMask[0]) | (CpuInfo->CpuidVerEax.Uint32 & ~DataMask[0]);
  Ebx.Uint32 = (Data[1] & DataMask[1]) | (CpuInfo->CpuidVerEbx.Uint32 & ~DataMask[1]);
  Ecx.Uint32 = (Data[2] & DataMask[2]) | (CpuInfo->CpuidVerEcx.Uint32 & ~DataMask[2]);
  Edx.Uint32 = (Data[3] & DataMask[3]) | (CpuInfo->CpuidVerEdx.Uint32 & ~DataMask[3]);

  if (FoundReleaseKernel) {
    CpuidPatch         = (INTERNAL_CPUID_PATCH *) Record;
    CpuidPatch->EaxCmd = 0xB8;
    CpuidPatch->EaxVal = Eax.Uint32;
    CpuidPatch->EbxCmd = 0xBB;
    CpuidPatch->EbxVal = Ebx.Uint32;
    CpuidPatch->EcxCmd = 0xB9;
    CpuidPatch->EcxVal = Ecx.Uint32;
    CpuidPatch->EdxCmd = 0xBA;
    CpuidPatch->EdxVal = Edx.Uint32;
    SetMem (
      Record + sizeof (INTERNAL_CPUID_PATCH),
      FoundSize - sizeof (INTERNAL_CPUID_PATCH),
      0x90
      );
    Record += FoundSize;

    for (Index = 0; Index < EFI_PAGE_SIZE - sizeof (mKernelCpuidFindMcRel); ++Index, ++Record) {
      if (CompareMem (Record, mKernelCpuidFindMcRel, sizeof (mKernelCpuidFindMcRel)) == 0) {
        McPatch         = (INTERNAL_MICROCODE_PATCH *) Record;
        McPatch->EdxCmd = 0xBA;
        McPatch->EdxVal = CpuInfo->MicrocodeRevision;
        SetMem (
          Record + sizeof (INTERNAL_MICROCODE_PATCH),
          sizeof (mKernelCpuidFindMcRel) - sizeof (INTERNAL_MICROCODE_PATCH),
          0x90
          );
        return EFI_SUCCESS;
      }
    }
  } else {
    //
    // Handle debug kernel here...
    //
    Status = PatcherGetSymbolAddress (Patcher, "_cpuid_set_cpufamily", (UINT8 **) &Record);
    if (EFI_ERROR (Status) || Record >= Last) {
      DEBUG ((DEBUG_WARN, "OCAK: Failed to locate _cpuid_set_cpufamily (%p) - %r\n", Record, Status));
      return EFI_NOT_FOUND;
    }

    CopyMem (Record, mKernelCpuidReplaceDbg, sizeof (mKernelCpuidReplaceDbg));
    FnPatch = (INTERNAL_CPUID_FN_PATCH *) Record;
    FnPatch->Signature     = Eax.Uint32;
    FnPatch->Stepping      = (UINT8) Eax.Bits.SteppingId;
    FnPatch->ExtModel      = (UINT8) Eax.Bits.ExtendedModelId;
    FnPatch->Model         = (UINT8) Eax.Bits.Model | (UINT8) (Eax.Bits.ExtendedModelId << 4U);
    FnPatch->Family        = (UINT8) Eax.Bits.FamilyId;
    FnPatch->Type          = (UINT8) Eax.Bits.ProcessorType;
    FnPatch->ExtFamily     = (UINT8) Eax.Bits.ExtendedFamilyId;
    FnPatch->Features      = LShiftU64 (Ecx.Uint32, 32) | (UINT64) Edx.Uint32;
    if (FnPatch->Features & CPUID_FEATURE_HTT) {
      FnPatch->LogicalPerPkg = (UINT16) Ebx.Bits.MaximumAddressableIdsForLogicalProcessors;
    } else {
      FnPatch->LogicalPerPkg = 1;
    }

    FnPatch->AppleFamily1 = FnPatch->AppleFamily2 = OcCpuModelToAppleFamily (Eax);

    return EFI_SUCCESS;
  }

  DEBUG ((DEBUG_WARN, "OCAK: Failed to find either CPUID patch (%u)\n", FoundSize));

  return EFI_UNSUPPORTED;
}

STATIC
CONST UINT8
mCustomSmbiosGuidPatchFind[] = {
  0x45, 0x42, 0x39, 0x44, 0x32, 0x44, 0x33, 0x31
};

STATIC
CONST UINT8
mCustomSmbiosGuidPatchReplace[] = {
  0x45, 0x42, 0x39, 0x44, 0x32, 0x44, 0x33, 0x35
};

STATIC
PATCHER_GENERIC_PATCH
mCustomSmbiosGuidPatch = {
  .Comment      = DEBUG_POINTER ("CustomSmbiosGuid"),
  .Base         = NULL,
  .Find         = mCustomSmbiosGuidPatchFind,
  .Mask         = NULL,
  .Replace      = mCustomSmbiosGuidPatchReplace,
  .ReplaceMask  = NULL,
  .Size         = sizeof (mCustomSmbiosGuidPatchFind),
  .Count        = 1,
  .Skip         = 0
};

STATIC
EFI_STATUS
PatchCustomSmbiosGuid (
  IN OUT PATCHER_CONTEXT    *Patcher,
  IN     UINT32             KernelVersion
  )
{
  EFI_STATUS          Status;

  if (Patcher == NULL) {
    return EFI_NOT_FOUND;
  }

  Status = PatcherApplyGenericPatch (Patcher, &mCustomSmbiosGuidPatch);
  if (!EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCAK: SMBIOS Patch success\n"));
  } else {
    DEBUG ((DEBUG_INFO, "OCAK: Failed to apply SMBIOS patch - %r\n", Status));
  }

  return Status;
}

STATIC
CONST UINT8
mPanicKextDumpPatchFind[] = {
  0x00, 0x25, 0x2E, 0x2A, 0x73, 0x00 ///< \0%.*s\0
};

STATIC
CONST UINT8
mPanicKextDumpPatchReplace[] = {
  0x00, 0x00, 0x2E, 0x2A, 0x73, 0x00
};

STATIC
PATCHER_GENERIC_PATCH
mPanicKextDumpPatch = {
  .Comment = DEBUG_POINTER ("PanicKextDump"),
  .Base    = NULL,
  .Find    = mPanicKextDumpPatchFind,
  .Mask    = NULL,
  .Replace = mPanicKextDumpPatchReplace,
  .ReplaceMask = NULL,
  .Size    = sizeof (mPanicKextDumpPatchFind),
  .Count   = 1,
  .Skip    = 0
};

STATIC
EFI_STATUS
PatchPanicKextDump (
  IN OUT PATCHER_CONTEXT    *Patcher,
  IN     UINT32             KernelVersion
  )
{
  EFI_STATUS          Status;
  UINT8               *Record;
  UINT8               *Last;

  ASSERT (Patcher != NULL);

  if (!OcMatchDarwinVersion (KernelVersion, KERNEL_VERSION_HIGH_SIERRA_MIN, 0)) {
    DEBUG ((DEBUG_INFO, "OCAK: Skipping XcpmCfgLock on %u\n", KernelVersion));
    return EFI_SUCCESS;
  }

  Last = ((UINT8 *) MachoGetMachHeader64 (&Patcher->MachContext)
    + MachoGetFileSize (&Patcher->MachContext) - EFI_PAGE_SIZE);

  //
  // This should work on 10.15 and all debug kernels.
  //
  Status = PatcherGetSymbolAddress (
    Patcher,
    "__ZN6OSKext19printKextPanicListsEPFiPKczE",
    (UINT8 **) &Record
    );
  if (EFI_ERROR (Status) || Record >= Last) {
    DEBUG ((DEBUG_WARN, "OCAK: Failed to locate printKextPanicLists (%p) - %r\n", Record, Status));
    return EFI_NOT_FOUND;
  }

  *Record = 0xC3;

  //
  // This one is for 10.13~10.14 release kernels, which do dumping inline.
  // A bit risky, but let's hope it works well.
  //
  Status = PatcherApplyGenericPatch (Patcher, &mPanicKextDumpPatch);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCAK: Failed to apply kext dump patch - %r\n", Status));
  } else {
    DEBUG ((DEBUG_INFO, "OCAK: Patch success kext dump\n"));
  }

  return EFI_SUCCESS;
}

STATIC
CONST UINT8
mLapicKernelPanicPatchFind[] = {
  // mov eax, gs:1Ch or gs:18h on 10.15.4+ or gs:20h on 11.0.
  0x65, 0x8B, 0x04, 0x25, 0x00, 0x00, 0x00, 0x00,
  // cmp eax, cs:_master_cpu <- address masked out
  0x3B, 0x00, 0x00, 0x00, 0x00, 0x00
};

STATIC
CONST UINT8
mLapicKernelPanicPatchMask[] = {
  0xFF, 0xFF, 0xFF, 0xFF, 0xC3, 0xFF, 0xFF, 0xFF,
  0xFF, 0x00, 0x00, 0x00, 0x00, 0x00
};

STATIC
CONST UINT8
mLapicKernelPanicPatchReplace[] = {
  // xor eax, eax
  0x31, 0xC0,
  // nop further
  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90
};

STATIC
PATCHER_GENERIC_PATCH
mLapicKernelPanicPatch = {
  .Comment = DEBUG_POINTER ("LapicKernelPanic"),
  .Base    = "_lapic_interrupt",
  .Find    = mLapicKernelPanicPatchFind,
  .Mask    = mLapicKernelPanicPatchMask,
  .Replace = mLapicKernelPanicPatchReplace,
  .ReplaceMask = NULL,
  .Size    = sizeof (mLapicKernelPanicPatchReplace),
  .Count   = 1,
  .Skip    = 0,
  .Limit   = 1024
};

STATIC
CONST UINT8
mLapicKernelPanicPatchLegacyFind[] = {
  // mov eax, gs:1Ch on 10.9.5 and 14h on 10.8.5.
  0x65, 0x8B, 0x04, 0x25, 0x10, 0x00, 0x00, 0x00,
  // lea rcx, _master_cpu
  0x48, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  // cmp eax, [rcx]
  0x00, 0x00
};

STATIC
CONST UINT8
mLapicKernelPanicPatchLegacyMask[] = {
  0xFF, 0xFF, 0xFF, 0xFF, 0xF3, 0xFF, 0xFF, 0xFF,
  0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00
};

STATIC
CONST UINT8
mLapicKernelPanicPatchLegacyReplace[] = {
  // xor eax, eax
  0x31, 0xC0,
  // nop further
  0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90
};

STATIC
PATCHER_GENERIC_PATCH
mLapicKernelPanicLegacyPatch = {
  .Comment = DEBUG_POINTER ("LapicKernelPanicLegacy"),
  .Base    = "_lapic_interrupt",
  .Find    = mLapicKernelPanicPatchLegacyFind,
  .Mask    = mLapicKernelPanicPatchLegacyMask,
  .Replace = mLapicKernelPanicPatchLegacyReplace,
  .ReplaceMask = NULL,
  .Size    = sizeof (mLapicKernelPanicPatchLegacyReplace),
  .Count   = 1,
  .Skip    = 0,
  .Limit   = 1024
};

STATIC
UINT8
mLapicKernelPanicMasterPatchFind[] = {
  // cmp cs:_debug_boot_arg, 0 <- address masked out
  0x83, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

STATIC
UINT8
mLapicKernelPanicMasterPatchMask[] = {
  0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF
};

STATIC
UINT8
mLapicKernelPanicMasterPatchReplace[] = {
  // xor eax, eax ; nop further
  0x31, 0xC0, 0x90, 0x90, 0x90, 0x90, 0x90
};

STATIC
PATCHER_GENERIC_PATCH
mLapicKernelPanicMasterPatch = {
  .Comment = DEBUG_POINTER ("LapicKernelPanicMaster"),
  .Base    = "_lapic_interrupt",
  .Find    = mLapicKernelPanicMasterPatchFind,
  .Mask    = mLapicKernelPanicMasterPatchMask,
  .Replace = mLapicKernelPanicMasterPatchReplace,
  .ReplaceMask = NULL,
  .Size    = sizeof (mLapicKernelPanicMasterPatchFind),
  .Count   = 1,
  .Skip    = 0,
  .Limit   = 4096
};

STATIC
EFI_STATUS
PatchLapicKernelPanic (
  IN OUT PATCHER_CONTEXT    *Patcher,
  IN     UINT32             KernelVersion
  )
{
  EFI_STATUS  Status;

  ASSERT (Patcher != NULL);

  //
  // This one is for <= 10.15 release kernels.
  // TODO: Fix debug kernels and check whether we want more patches.
  //
  Status = PatcherApplyGenericPatch (Patcher, &mLapicKernelPanicPatch);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCAK: Failed to apply modern lapic patch - %r\n", Status));

    Status = PatcherApplyGenericPatch (Patcher, &mLapicKernelPanicLegacyPatch);
    if (!EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "OCAK: Patch success legacy lapic\n"));
    } else {
      DEBUG ((DEBUG_INFO, "OCAK: Failed to apply modern lapic patch - %r\n", Status));
    }
  } else {
    DEBUG ((DEBUG_INFO, "OCAK: Patch success lapic\n"));

    //
    // Also patch away the master core check to never require lapic_dont_panic=1.
    // This one is optional, and seems to never be required in real world.
    //
    Status = PatcherApplyGenericPatch (Patcher, &mLapicKernelPanicMasterPatch);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "OCAK: Failed to apply extended lapic patch - %r\n", Status));
    } else {
      DEBUG ((DEBUG_INFO, "OCAK: Patch success extended lapic\n"));
    }

    Status = EFI_SUCCESS;
  }

  return Status;
}

STATIC
UINT8
mPowerStateTimeoutPanicFind[] = {
  // com.apple\0
  0x63, 0x6F, 0x6D, 0x2E, 0x61, 0x70, 0x70, 0x6C, 0x65, 0x00
};

STATIC
UINT8
mPowerStateTimeoutPanicReplace[] = {
  // not.apple\0
  0x6E, 0x6F, 0x74, 0x2E, 0x61, 0x70, 0x70, 0x6C, 0x65, 0x00
};

STATIC
PATCHER_GENERIC_PATCH
mPowerStateTimeoutPanicMasterPatch = {
  .Comment     = DEBUG_POINTER ("PowerStateTimeout"),
  .Base        = NULL,
  .Find        = mPowerStateTimeoutPanicFind,
  .Mask        = NULL,
  .Replace     = mPowerStateTimeoutPanicReplace,
  .ReplaceMask = NULL,
  .Size        = sizeof (mPowerStateTimeoutPanicFind),
  .Count       = 1,
  .Skip        = 0,
  .Limit       = 0
};

STATIC
EFI_STATUS
PatchPowerStateTimeout (
  IN OUT PATCHER_CONTEXT    *Patcher,
  IN     UINT32             KernelVersion
  )
{
  EFI_STATUS  Status;

  ASSERT (Patcher != NULL);

  if (!OcMatchDarwinVersion (KernelVersion, KERNEL_VERSION_CATALINA_MIN, 0)) {
    DEBUG ((DEBUG_INFO, "OCAK: Skipping power state patch on %u\n", KernelVersion));
    return EFI_SUCCESS;
  }

  Status = PatcherApplyGenericPatch (Patcher, &mPowerStateTimeoutPanicMasterPatch);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCAK: Failed to apply power state patch - %r\n", Status));
  } else {
    DEBUG ((DEBUG_INFO, "OCAK: Patch success power state\n"));
  }

  //
  // TODO: Implement a patch to not require setpowerstate_panic=0 on debug kernels.
  //

  return Status;
}

//
// There currently are 2 places main RTC checksum is calculated in AppleRTC.kext
// __ZN8AppleRTC14updateChecksumEv and __ZN8AppleRTC19rtcRecordTracePointEjjj.
// Since we do not want to completely break RTC and/or saving tracepoints to RTC
// we patch-out __ZN8AppleRTC8rtcWriteEjh call arguments (0x58 and 0x59) with
// invalid (out of range) value 0xFFFF in 4 places.
//

STATIC
UINT8
mAppleRtcChecksumPatchFind[] = {
  0xBE, 0x58, 0x00, 0x00, 0x00
};

STATIC
UINT8
mAppleRtcChecksumPatchMask[] = {
  0xFF, 0xFE, 0xFF, 0xFF, 0xFF
};

STATIC
UINT8
mAppleRtcChecksumPatchReplace[] = {
  0xBE, 0xFF, 0xFF, 0x00, 0x00
};

STATIC
PATCHER_GENERIC_PATCH
mAppleRtcChecksumPatch = {
  .Comment     = DEBUG_POINTER ("DisableRtcChecksum"),
  .Base        = NULL,
  .Find        = mAppleRtcChecksumPatchFind,
  .Mask        = mAppleRtcChecksumPatchMask,
  .Replace     = mAppleRtcChecksumPatchReplace,
  .ReplaceMask = NULL,
  .Size        = sizeof (mAppleRtcChecksumPatchFind),
  .Count       = 4,
  .Skip        = 0,
  .Limit       = 0
};

STATIC
EFI_STATUS
PatchAppleRtcChecksum (
  IN OUT PATCHER_CONTEXT    *Patcher,
  IN     UINT32             KernelVersion
  )
{
  EFI_STATUS Status;

  if (Patcher == NULL) {
    return EFI_NOT_FOUND;
  }

  Status = PatcherApplyGenericPatch (Patcher, &mAppleRtcChecksumPatch);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCAK: Failed to apply patch com.apple.driver.AppleRTC DisableRtcChecksum - %r\n", Status));
  } else {
    DEBUG ((DEBUG_INFO, "OCAK: Patch success com.apple.driver.AppleRTC DisableRtcChecksum\n"));
  }

  return Status;
}

STATIC
EFI_STATUS
PatchSegmentJettison (
  IN OUT PATCHER_CONTEXT    *Patcher,
  IN     UINT32             KernelVersion
  )
{
  EFI_STATUS  Status;
  UINT8       *RemoveBs;
  UINT8       *StaticMfree;
  UINT8       *CurrFreeCall;
  CHAR8       *Jettisoning;
  UINT8       *Last;
  UINTN       Index;
  UINT32      Diff;
  UINT32      Diff2;

  if (!OcMatchDarwinVersion (KernelVersion, KERNEL_VERSION_BIG_SUR_MIN, 0)) {
    DEBUG ((DEBUG_INFO, "OCAK: Skipping SegmentJettison on %u\n", KernelVersion));
    return EFI_SUCCESS;
  }

  Last = (UINT8 *) MachoGetMachHeader64 (&Patcher->MachContext)
    + MachoGetFileSize (&Patcher->MachContext) - sizeof (EFI_PAGE_SIZE) * 2;

  Status = PatcherGetSymbolAddress (Patcher, "__ZN6OSKext19removeKextBootstrapEv", (UINT8 **) &RemoveBs);
  if (EFI_ERROR (Status) || RemoveBs > Last) {
    DEBUG ((DEBUG_INFO, "OCAK: Missing removeKextBootstrap - %r\n", Status));
    return EFI_NOT_FOUND;
  }

  Status = PatcherGetSymbolAddress (Patcher, "_ml_static_mfree", (UINT8 **) &StaticMfree);
  if (EFI_ERROR (Status) || RemoveBs > Last) {
    DEBUG ((DEBUG_INFO, "OCAK: Missing ml_static_mfree - %r\n", Status));
    return EFI_NOT_FOUND;
  }

  if (RemoveBs - StaticMfree > MAX_INT32) {
    DEBUG ((DEBUG_INFO, "OCAK: ml_static_mfree %p removeKextBootstrap %p are too far\n", StaticMfree, RemoveBs));
    return EFI_UNSUPPORTED;
  }

  //
  // Find the call to _ml_static_mfree.
  //
  Diff = (UINT32)((UINTN)StaticMfree - (UINTN)RemoveBs - 5);

  CurrFreeCall = NULL;
  for (Index = 0; Index < EFI_PAGE_SIZE; ++Index) {
    if (RemoveBs[0] == 0xE8
      && CompareMem (&RemoveBs[1], &Diff, sizeof (Diff)) == 0) {
      CurrFreeCall = RemoveBs;
      DEBUG ((
        DEBUG_VERBOSE,
        "OCAK: CurrFreeCall %02X %02X %02X %02X %02X %X\n",
        RemoveBs[0],
        RemoveBs[1],
        RemoveBs[2],
        RemoveBs[3],
        RemoveBs[4],
        Diff
        ));
    } else if (CurrFreeCall != NULL
      && RemoveBs[0] == 0x48 && RemoveBs[1] == 0x8D && RemoveBs[2] == 0x15) {
      //
      // Check if this lea rdx, address is pointing to "Jettisoning fileset Linkedit segments from..."
      //
      CopyMem (&Diff2, &RemoveBs[3], sizeof (Diff2));
      Jettisoning = (CHAR8 *) RemoveBs + Diff2 + 7;
      if ((UINT8 *) Jettisoning <= Last
        && AsciiStrnCmp (Jettisoning, "Jettisoning fileset", L_STR_LEN ("Jettisoning fileset")) == 0) {
        DEBUG ((DEBUG_INFO, "OCAK: Found jettisoning fileset\n"));
        SetMem (CurrFreeCall, 5, 0x90);
        return EFI_SUCCESS;    
      }
    }

    ++RemoveBs;
    --Diff;
  }

  DEBUG ((DEBUG_INFO, "OCAK: Failed to find jettisoning fileset - %p\n", CurrFreeCall));

  return EFI_NOT_FOUND;
}

//
// Quirks table.
//
KERNEL_QUIRK gKernelQuirks[] = {
  [KernelQuirkAppleCpuPmCfgLock] = { "com.apple.driver.AppleIntelCPUPowerManagement", PatchAppleCpuPmCfgLock }, 
  [KernelQuirkAppleXcpmCfgLock] = { NULL, PatchAppleXcpmCfgLock },
  [KernelQuirkAppleXcpmExtraMsrs] = { NULL, PatchAppleXcpmExtraMsrs },
  [KernelQuirkAppleXcpmForceBoost] = { NULL, PatchAppleXcpmForceBoost },
  [KernelQuirkCustomSmbiosGuid1] = { "com.apple.driver.AppleSMBIOS", PatchCustomSmbiosGuid },
  [KernelQuirkCustomSmbiosGuid2] = { "com.apple.driver.AppleACPIPlatform", PatchCustomSmbiosGuid },
  [KernelQuirkDisableIoMapper] = { "com.apple.iokit.IOPCIFamily", PatchAppleIoMapperSupport },
  [KernelQuirkDisableRtcChecksum] = { "com.apple.driver.AppleRTC", PatchAppleRtcChecksum },
  [KernelQuirkDummyPowerManagement] = { "com.apple.driver.AppleIntelCPUPowerManagement", PatchDummyPowerManagement },
  [KernelQuirkExternalDiskIcons] = { "com.apple.driver.AppleAHCIPort", PatchForceInternalDiskIcons },
  [KernelQuirkIncreasePciBarSize] = { "com.apple.iokit.IOPCIFamily", PatchIncreasePciBarSize },
  [KernelQuirkLapicKernelPanic] = { NULL, PatchLapicKernelPanic },
  [KernelQuirkPanicNoKextDump] = { NULL, PatchPanicKextDump },
  [KernelQuirkPowerTimeoutKernelPanic] = { NULL, PatchPowerStateTimeout },
  [KernelQuirkThirdPartyDrives] = { "com.apple.iokit.IOAHCIBlockStorage", PatchThirdPartyDriveSupport },
  [KernelQuirkXhciPortLimit1] = { "com.apple.iokit.IOUSBHostFamily", PatchUsbXhciPortLimit1 },
  [KernelQuirkXhciPortLimit2] = { "com.apple.driver.usb.AppleUSBXHCI", PatchUsbXhciPortLimit2 },
  [KernelQuirkXhciPortLimit3] = { "com.apple.driver.usb.AppleUSBXHCIPCI", PatchUsbXhciPortLimit3 },
  [KernelQuirkSegmentJettison] = { NULL, PatchSegmentJettison },
};

EFI_STATUS
KernelApplyQuirk (
  IN     KERNEL_QUIRK_NAME  Name,
  IN OUT PATCHER_CONTEXT    *Patcher,
  IN     UINT32             KernelVersion
  )
{
  ASSERT (Patcher != NULL);

  return gKernelQuirks[Name].PatchFunction (Patcher, KernelVersion);
}
