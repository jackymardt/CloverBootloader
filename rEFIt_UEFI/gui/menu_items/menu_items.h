/*
 *
 * Copyright (c) 2020 Jief
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the
 *    distribution.
 *
 *  * Neither the name of Christoph Pfisterer nor the names of the
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __menu_items_H__
#define __menu_items_H__


#include "../../libeg/libeg.h"
#include "../../refit/lib.h"
#include "../../Platform/LoaderUefi.h"

#include "../../cpp_foundation/XObjArray.h"
#include "../../cpp_foundation/XStringArray.h"
#include "../../cpp_foundation/XString.h"
#include "../../libeg/XPointer.h"


//
//#define REFIT_DEBUG (2)
//#define Print if ((!GlobalConfig.Quiet) || (GlobalConfig.TextOnly)) Print
////#include "GenericBdsLib.h"


//#define TAG_ABOUT_OLD              (1)
//#define TAG_RESET_OLD              (2)
//#define TAG_SHUTDOWN_OLD           (3)
//#define TAG_TOOL_OLD               (4)
////#define TAG_LOADER             (5)
////#define TAG_LEGACY             (6)
//#define TAG_INFO_OLD               (7)
//#define TAG_OPTIONS            (8)
//#define TAG_INPUT_OLD              (9)
//#define TAG_HELP_OLD               (10) // wasn't used ?
//#define TAG_SWITCH_OLD             (11)
//#define TAG_CHECKBIT_OLD           (12)
//#define TAG_SECURE_BOOT_OLD        (13)
//#define TAG_SECURE_BOOT_CONFIG_OLD (14)
//#define TAG_CLOVER_OLD             (100)
//#define TAG_EXIT_OLD               (101)
//#define TAG_RETURN_OLD             ((UINTN)(-1))

//typedef struct _refit_menu_screen REFIT_MENU_SCREEN;
class REFIT_MENU_SCREEN;
class REFIT_MENU_SWITCH;
class REFIT_MENU_CHECKBIT;
class REFIT_MENU_ENTRY_CLOVER;
class REFIT_MENU_ITEM_RETURN;
class REFIT_INPUT_DIALOG;
class REFIT_INFO_DIALOG;
class REFIT_MENU_ENTRY_LOADER_TOOL;
class REFIT_MENU_ITEM_SHUTDOWN;
class REFIT_MENU_ITEM_RESET;
class REFIT_MENU_ITEM_ABOUT;
class REFIT_MENU_ITEM_OPTIONS;
class REFIT_MENU_ITEM_ABSTRACT_ENTRY_LOADER;
class LOADER_ENTRY;
class LEGACY_ENTRY;
class REFIT_MENU_ENTRY_OTHER;
class REFIT_SIMPLE_MENU_ENTRY_TAG;
class REFIT_MENU_ENTRY_ITEM_ABSTRACT;
class REFIT_MENU_ITEM_BOOTNUM;
class XPointer;

/**********************************************************  REFIT_ABSTRACT_MENU_ENTRY  *************************************************************/

class REFIT_ABSTRACT_MENU_ENTRY
{
  public:
  XStringW          Title;
  UINTN              Row;
  CHAR16             ShortcutDigit;
  CHAR16             ShortcutLetter;
  XImage            Image;
  EG_RECT            Place;
  ACTION             AtClick;
  ACTION             AtDoubleClick;
  ACTION             AtRightClick;
  ACTION             AtMouseOver;
  REFIT_MENU_SCREEN *SubScreen;

  virtual XImage* getDriveImage() { return nullptr; };
  virtual XImage* getBadgeImage() { return nullptr; };

  virtual REFIT_SIMPLE_MENU_ENTRY_TAG* getREFIT_SIMPLE_MENU_ENTRY_TAG() { return nullptr; };
  virtual REFIT_MENU_SWITCH* getREFIT_MENU_SWITCH() { return nullptr; };
  virtual REFIT_MENU_CHECKBIT* getREFIT_MENU_CHECKBIT() { return nullptr; };
  virtual REFIT_MENU_ENTRY_CLOVER* getREFIT_MENU_ENTRY_CLOVER() { return nullptr; };
  virtual REFIT_MENU_ITEM_RETURN* getREFIT_MENU_ITEM_RETURN() { return nullptr; };
  virtual REFIT_INPUT_DIALOG* getREFIT_INPUT_DIALOG() { return nullptr; };
  virtual REFIT_INFO_DIALOG* getREFIT_INFO_DIALOG() { return nullptr; };
  virtual REFIT_MENU_ENTRY_LOADER_TOOL* getREFIT_MENU_ENTRY_LOADER_TOOL() { return nullptr; };
  virtual REFIT_MENU_ITEM_SHUTDOWN* getREFIT_MENU_ITEM_SHUTDOWN() { return nullptr; };
  virtual REFIT_MENU_ITEM_RESET* getREFIT_MENU_ITEM_RESET() { return nullptr; };
  virtual REFIT_MENU_ITEM_ABOUT* getREFIT_MENU_ITEM_ABOUT() { return nullptr; };
  virtual REFIT_MENU_ITEM_OPTIONS* getREFIT_MENU_ITEM_OPTIONS() { return nullptr; };
  virtual REFIT_MENU_ITEM_ABSTRACT_ENTRY_LOADER* getREFIT_MENU_ITEM_ABSTRACT_ENTRY_LOADER() { return nullptr; };
  virtual LOADER_ENTRY* getLOADER_ENTRY() { return nullptr; };
  virtual LEGACY_ENTRY* getLEGACY_ENTRY() { return nullptr; };
  virtual REFIT_MENU_ENTRY_OTHER* getREFIT_MENU_ENTRY_OTHER() { return nullptr; };
  virtual REFIT_MENU_ENTRY_ITEM_ABSTRACT* getREFIT_MENU_ITEM_IEM_ABSTRACT() { return nullptr; };
  virtual REFIT_MENU_ITEM_BOOTNUM* getREFIT_MENU_ITEM_BOOTNUM() { return nullptr; };
  virtual void StartLoader() {};
  virtual void StartLegacy() {};
  virtual void StartTool() {};

  REFIT_ABSTRACT_MENU_ENTRY() : Row(0), ShortcutDigit(0), ShortcutLetter(0), Image(), AtClick(ActionNone), AtDoubleClick(ActionNone), AtRightClick(ActionNone), AtMouseOver(ActionNone), SubScreen(NULL)
  {};
  REFIT_ABSTRACT_MENU_ENTRY(const XStringW& Title_) : Title(Title_), Row(0), ShortcutDigit(0), ShortcutLetter(0), Image(), AtClick(ActionNone), AtDoubleClick(ActionNone), AtRightClick(ActionNone), AtMouseOver(ActionNone), SubScreen(NULL)
  {};
  REFIT_ABSTRACT_MENU_ENTRY(const XStringW& Title_, ACTION AtClick_) : Title(Title_), Row(0), ShortcutDigit(0), ShortcutLetter(0), Image(), AtClick(AtClick_), AtDoubleClick(ActionNone), AtRightClick(ActionNone), AtMouseOver(ActionNone), SubScreen(NULL)
  {};
  REFIT_ABSTRACT_MENU_ENTRY(const XStringW& Title_, UINTN Row_, CHAR16 ShortcutDigit_, CHAR16 ShortcutLetter_, ACTION AtClick_)
  : Title(Title_), Row(Row_), ShortcutDigit(ShortcutDigit_), ShortcutLetter(ShortcutLetter_), Image(), AtClick(AtClick_), AtDoubleClick(ActionNone), AtRightClick(ActionNone), AtMouseOver(ActionNone), SubScreen(NULL)
  {};
  REFIT_ABSTRACT_MENU_ENTRY(const XStringW& Title_, UINTN Row_,
                            CHAR16 ShortcutDigit_, CHAR16 ShortcutLetter_, const XImage& Image_,
                            EG_RECT Place_, ACTION AtClick_, ACTION AtDoubleClick_, ACTION AtRightClick_, ACTION AtMouseOver_,
                            REFIT_MENU_SCREEN *SubScreen_)
  : Title(Title_), Row(Row_), ShortcutDigit(ShortcutDigit_), ShortcutLetter(ShortcutLetter_),
  Image(Image_), Place(Place_),
  AtClick(AtClick_), AtDoubleClick(AtDoubleClick_), AtRightClick(AtRightClick_), AtMouseOver(AtMouseOver_),
  SubScreen(SubScreen_) {};

  virtual ~REFIT_ABSTRACT_MENU_ENTRY() {}; // virtual destructor : this is vital
};



/**********************************************************  REFIT_ABSTRACT_MENU_ENTRY  *************************************************************/

	class REFIT_SIMPLE_MENU_ENTRY_TAG : public REFIT_ABSTRACT_MENU_ENTRY
	{
	public:
	  UINTN              Tag;

	  REFIT_SIMPLE_MENU_ENTRY_TAG(const XStringW& Title_, UINTN Tag_, ACTION AtClick_)
				 : REFIT_ABSTRACT_MENU_ENTRY(Title_, AtClick_), Tag(Tag_)
				 {};

	  virtual REFIT_SIMPLE_MENU_ENTRY_TAG* getREFIT_SIMPLE_MENU_ENTRY_TAG() { return this; };
	};



/**********************************************************  Simple entries. Inherit from REFIT_ABSTRACT_MENU_ENTRY  *************************************************************/

	class REFIT_MENU_ITEM_RETURN : public REFIT_ABSTRACT_MENU_ENTRY
	{
	public:
	  REFIT_MENU_ITEM_RETURN() : REFIT_ABSTRACT_MENU_ENTRY() {};
	  REFIT_MENU_ITEM_RETURN(const XStringW& Title_, UINTN Row_, CHAR16 ShortcutDigit_, CHAR16 ShortcutLetter_, ACTION AtClick_)
				 : REFIT_ABSTRACT_MENU_ENTRY(Title_, Row_, ShortcutDigit_, ShortcutLetter_, AtClick_)
				 {};
	  virtual REFIT_MENU_ITEM_RETURN* getREFIT_MENU_ITEM_RETURN() { return this; };
	};

	class REFIT_MENU_ITEM_SHUTDOWN : public REFIT_ABSTRACT_MENU_ENTRY
	{
	public:
	  REFIT_MENU_ITEM_SHUTDOWN() : REFIT_ABSTRACT_MENU_ENTRY() {};
	  REFIT_MENU_ITEM_SHUTDOWN(const XStringW& Title_, UINTN Row_, CHAR16 ShortcutDigit_, CHAR16 ShortcutLetter_, ACTION AtClick_)
				 : REFIT_ABSTRACT_MENU_ENTRY(Title_, Row_, ShortcutDigit_, ShortcutLetter_, AtClick_)
				 {};
	  virtual REFIT_MENU_ITEM_SHUTDOWN* getREFIT_MENU_ITEM_SHUTDOWN() { return this; };
	};

	class REFIT_MENU_ITEM_RESET : public REFIT_ABSTRACT_MENU_ENTRY {
	public:
	  REFIT_MENU_ITEM_RESET() : REFIT_ABSTRACT_MENU_ENTRY() {};
	  REFIT_MENU_ITEM_RESET(const XStringW& Title_, UINTN Row_, CHAR16 ShortcutDigit_, CHAR16 ShortcutLetter_, ACTION AtClick_)
				 : REFIT_ABSTRACT_MENU_ENTRY(Title_, Row_, ShortcutDigit_, ShortcutLetter_, AtClick_)
				 {};
	  virtual REFIT_MENU_ITEM_RESET* getREFIT_MENU_ITEM_RESET() { return this; };
	};

	class REFIT_MENU_ITEM_ABOUT : public REFIT_ABSTRACT_MENU_ENTRY
	{
	public:
	  REFIT_MENU_ITEM_ABOUT() : REFIT_ABSTRACT_MENU_ENTRY() {};
	  REFIT_MENU_ITEM_ABOUT(const XStringW& Title_, UINTN Row_, CHAR16 ShortcutDigit_, CHAR16 ShortcutLetter_, ACTION AtClick_)
				 : REFIT_ABSTRACT_MENU_ENTRY(Title_, Row_, ShortcutDigit_, ShortcutLetter_, AtClick_)
				 {};
	  virtual REFIT_MENU_ITEM_ABOUT* getREFIT_MENU_ITEM_ABOUT() { return this; };
	};

	class REFIT_MENU_ITEM_OPTIONS : public REFIT_ABSTRACT_MENU_ENTRY {
	public:
	  REFIT_MENU_ITEM_OPTIONS() : REFIT_ABSTRACT_MENU_ENTRY() {};
	  REFIT_MENU_ITEM_OPTIONS(const XStringW& Title_, UINTN Row_, CHAR16 ShortcutDigit_, CHAR16 ShortcutLetter_, ACTION AtClick_)
				 : REFIT_ABSTRACT_MENU_ENTRY(Title_, Row_, ShortcutDigit_, ShortcutLetter_, AtClick_)
				 {};
	  virtual REFIT_MENU_ITEM_OPTIONS* getREFIT_MENU_ITEM_OPTIONS() { return this; };
	};


	class REFIT_INFO_DIALOG : public REFIT_ABSTRACT_MENU_ENTRY {
	public:
	  virtual REFIT_INFO_DIALOG* getREFIT_INFO_DIALOG() { return this; };
	};



/**********************************************************    *************************************************************/

	class REFIT_MENU_ENTRY_ITEM_ABSTRACT : public REFIT_ABSTRACT_MENU_ENTRY {
	public:
	  INPUT_ITEM        *Item;
	  REFIT_MENU_ENTRY_ITEM_ABSTRACT() : Item(0) {}
	  virtual REFIT_MENU_ENTRY_ITEM_ABSTRACT* getREFIT_MENU_ITEM_IEM_ABSTRACT() { return this; };
	};

		/* Classes that needs a Item member */
		class REFIT_INPUT_DIALOG : public REFIT_MENU_ENTRY_ITEM_ABSTRACT {
		public:
		  virtual REFIT_INPUT_DIALOG* getREFIT_INPUT_DIALOG() { return this; };
		};

		class REFIT_MENU_SWITCH : public REFIT_MENU_ENTRY_ITEM_ABSTRACT {
		public:
		  virtual REFIT_MENU_SWITCH* getREFIT_MENU_SWITCH() { return this; };
		};

		class REFIT_MENU_CHECKBIT : public REFIT_MENU_ENTRY_ITEM_ABSTRACT {
		public:
		  virtual REFIT_MENU_CHECKBIT* getREFIT_MENU_CHECKBIT() { return this; };
		};

/**********************************************************  Loader entries  *************************************************************/
	/*
	 * Super class of LOADER_ENTRY & LEGACY_ENTRY
	 */
	class REFIT_MENU_ITEM_ABSTRACT_ENTRY_LOADER : public REFIT_ABSTRACT_MENU_ENTRY
	{
	public:
	  CONST CHAR16     *DevicePathString;
	  XStringArray          LoadOptions; //moved here for compatibility with legacy
	  XStringW    LoaderPath;
    XImage        DriveImage;
    XImage        BadgeImage;

    REFIT_MENU_ITEM_ABSTRACT_ENTRY_LOADER()
    : REFIT_ABSTRACT_MENU_ENTRY(), DevicePathString(0), DriveImage(), BadgeImage()
    {}
    virtual  XImage* getDriveImage()  { return &DriveImage; };
    virtual  XImage* getBadgeImage()  { return &BadgeImage; };

	  virtual REFIT_MENU_ITEM_ABSTRACT_ENTRY_LOADER* getREFIT_MENU_ITEM_ABSTRACT_ENTRY_LOADER() { return this; };
	};

		//---------------------------------------  REFIT_MENU_ENTRY_LOADER_TOOL  ---------------------------------------//

		class REFIT_MENU_ENTRY_LOADER_TOOL : public REFIT_MENU_ITEM_ABSTRACT_ENTRY_LOADER
		{
		  public:
			UINT8 NoMemset;
			UINT16            Flags;
			EFI_DEVICE_PATH  *DevicePath;
      
      VOID              StartTool();

			REFIT_MENU_ENTRY_LOADER_TOOL() : REFIT_MENU_ITEM_ABSTRACT_ENTRY_LOADER(), NoMemset(1), Flags(0), DevicePath(0) {};

			virtual REFIT_MENU_ENTRY_LOADER_TOOL* getREFIT_MENU_ENTRY_LOADER_TOOL() { return this; };
		};


		//---------------------------------------  REFIT_MENU_ENTRY_CLOVER  ---------------------------------------//

		class REFIT_MENU_ENTRY_CLOVER : public REFIT_MENU_ITEM_ABSTRACT_ENTRY_LOADER
		{
		  public:
			REFIT_VOLUME     *Volume;
			CONST CHAR16     *VolName;
			EFI_DEVICE_PATH  *DevicePath;
			UINT16            Flags;

			REFIT_MENU_ENTRY_CLOVER() : REFIT_MENU_ITEM_ABSTRACT_ENTRY_LOADER(), Volume(NULL), VolName(NULL), DevicePath(NULL), Flags(0)  {};

			REFIT_MENU_ENTRY_CLOVER* getPartiallyDuplicatedEntry() const;
			virtual REFIT_MENU_ENTRY_CLOVER* getREFIT_MENU_ENTRY_CLOVER() { return this; };
		};


		//---------------------------------------  REFIT_MENU_ITEM_BOOTNUM  ---------------------------------------//

		class REFIT_MENU_ITEM_BOOTNUM : public REFIT_MENU_ITEM_ABSTRACT_ENTRY_LOADER
		{
		  public:
			REFIT_VOLUME     *Volume;
			UINTN             BootNum;

			REFIT_MENU_ITEM_BOOTNUM() : REFIT_MENU_ITEM_ABSTRACT_ENTRY_LOADER(), Volume(NULL), BootNum(0) {};
			virtual REFIT_MENU_ITEM_BOOTNUM* getREFIT_MENU_ITEM_BOOTNUM() { return this; };
		} ;


			//---------------------------------------  LEGACY_ENTRY  ---------------------------------------//

			class LEGACY_ENTRY : public REFIT_MENU_ITEM_BOOTNUM
			{
			  public:
				LEGACY_ENTRY() : REFIT_MENU_ITEM_BOOTNUM() {};
        
        VOID StartLegacy();

				virtual LEGACY_ENTRY* getLEGACY_ENTRY() { return this; };
			};


			//---------------------------------------  LOADER_ENTRY  ---------------------------------------//

			struct KERNEL_AND_KEXT_PATCHES;

			class LOADER_ENTRY : public REFIT_MENU_ITEM_BOOTNUM
			{
			  public:
				CONST CHAR16     *VolName;
				EFI_DEVICE_PATH  *DevicePath;
				UINT16            Flags;
				UINT8             LoaderType;
				CHAR8            *OSVersion;
				CHAR8            *BuildVersion;
        EFI_GRAPHICS_OUTPUT_BLT_PIXEL BootBgColor;

				UINT8             CustomBoot;
				XImage            CustomLogo;
				KERNEL_AND_KEXT_PATCHES *KernelAndKextPatches;
				CONST CHAR16            *Settings;
        UINT32            AddrVtable;
        UINT32            SizeVtable;
        UINT32            NamesTable;

				LOADER_ENTRY()
						: REFIT_MENU_ITEM_BOOTNUM(), VolName(0), DevicePath(0), Flags(0), LoaderType(0), OSVersion(0), BuildVersion(0),
              BootBgColor({0,0,0,0}),
              CustomBoot(0), KernelAndKextPatches(0), Settings(0),
              AddrVtable(0), SizeVtable(0), NamesTable(0)
						{};
        
        VOID          FindBootArgs();
        EFI_STATUS    getVTable(UINT8* kernel);
        UINTN         searchProc(UINT8 * kernel, const char *procedure, UINTN *procLen);
        VOID          KernelAndKextsPatcherStart();
        VOID          KernelAndKextPatcherInit();
        BOOLEAN       KernelUserPatch(UINT8 * kernel);
        BOOLEAN       KernelPatchPm(VOID *kernelData);
        BOOLEAN       KernelLapicPatch_32(VOID *kernelData);
        BOOLEAN       KernelLapicPatch_64(VOID *kernelData);
        BOOLEAN       BooterPatch(IN UINT8 *BooterData, IN UINT64 BooterSize);
        VOID EFIAPI   KernelBooterExtensionsPatch(IN UINT8 *Kernel);
        BOOLEAN       KernelPanicNoKextDump(VOID *kernelData);
        VOID          KernelCPUIDPatch(UINT8* kernelData);
        BOOLEAN       PatchCPUID(UINT8* bytes, UINT8* Location, INT32 LenLoc,
                                 UINT8* Search4, UINT8* Search10, UINT8* ReplaceModel,
                                 UINT8* ReplaceExt, INT32 Len);
        VOID          KernelPatcher_32(VOID* kernelData, CHAR8 *OSVersion);
        VOID          KernelPatcher_64(VOID* kernelData);
        VOID          FilterKernelPatches();
        VOID          FilterKextPatches();
        VOID          FilterBootPatches();
        
        EFI_STATUS    SetFSInjection();
        EFI_STATUS    InjectKexts(IN UINT32 deviceTreeP, IN UINT32 *deviceTreeLength);
        EFI_STATUS    LoadKexts();
        int           is_mkext_v1(UINT8* drvPtr);
        void          patch_mkext_v1(UINT8 *drvPtr);
        // why EFIAPI?
        EFI_STATUS EFIAPI LoadKext(IN EFI_FILE *RootDir, IN CHAR16 *FileName, IN cpu_type_t archCpuType, IN OUT VOID *kext);
        EFI_STATUS EFIAPI AddKext(IN EFI_FILE *RootDir, IN CHAR16 *FileName, IN cpu_type_t archCpuType);
        VOID      LoadPlugInKexts(IN EFI_FILE *RootDir, IN CHAR16 *DirName, IN cpu_type_t archCpuType, IN BOOLEAN Force);
        VOID      AddKexts(CONST CHAR16 *SrcDir, CONST CHAR16 *Path, cpu_type_t archCpuType);
        VOID      KextPatcherRegisterKexts(void *FSInject, void *ForceLoadKexts);
        VOID      KextPatcherStart();
        VOID      PatchPrelinkedKexts();
        VOID      PatchLoadedKexts();
        VOID      PatchKext(UINT8 *Driver, UINT32 DriverSize, CHAR8 *InfoPlist, UINT32 InfoPlistSize);
        VOID      AnyKextPatch(UINT8 *Driver, UINT32 DriverSize, CHAR8 *InfoPlist, UINT32 InfoPlistSize, INT32 N);
        VOID      ATIConnectorsPatchInit();
        VOID      ATIConnectorsPatch(UINT8 *Driver, UINT32 DriverSize, CHAR8 *InfoPlist, UINT32 InfoPlistSize);
        VOID      ATIConnectorsPatchRegisterKexts(void *FSInject_v, void *ForceLoadKexts_v);
        VOID      AppleIntelCPUPMPatch(UINT8 *Driver, UINT32 DriverSize, CHAR8 *InfoPlist, UINT32 InfoPlistSize);
        VOID      AppleRTCPatch(UINT8 *Driver, UINT32 DriverSize, CHAR8 *InfoPlist, UINT32 InfoPlistSize);
        VOID      CheckForFakeSMC(CHAR8 *InfoPlist);
        VOID      DellSMBIOSPatch(UINT8 *Driver, UINT32 DriverSize, CHAR8 *InfoPlist, UINT32 InfoPlistSize);
        VOID      SNBE_AICPUPatch(UINT8 *Driver, UINT32 DriverSize, CHAR8 *InfoPlist, UINT32 InfoPlistSize);
        VOID      BDWE_IOPCIPatch(UINT8 *Driver, UINT32 DriverSize, CHAR8 *InfoPlist, UINT32 InfoPlistSize);
        BOOLEAN   SandyBridgeEPM(VOID *kernelData, BOOLEAN use_xcpm_idle);
        BOOLEAN   HaswellEXCPM(VOID *kernelData, BOOLEAN use_xcpm_idle);
        BOOLEAN   HaswellLowEndXCPM(VOID *kernelData, BOOLEAN use_xcpm_idle);
        BOOLEAN   BroadwellEPM(VOID *kernelData, BOOLEAN use_xcpm_idle);
        BOOLEAN   KernelIvyBridgeXCPM(VOID *kernelData, BOOLEAN use_xcpm_idle);
        BOOLEAN   KernelIvyE5XCPM(VOID *kernelData, BOOLEAN use_xcpm_idle);
        
        VOID Stall(int Pause) { if ((KernelAndKextPatches != NULL) && KernelAndKextPatches->KPDebug) { gBS->Stall(Pause); } };
        VOID StartLoader();
				LOADER_ENTRY* getPartiallyDuplicatedEntry() const;
				virtual LOADER_ENTRY* getLOADER_ENTRY() { return this; };
			} ;


#endif
/*
 
 EOF */
