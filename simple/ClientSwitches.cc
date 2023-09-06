#include "ClientSwitches.h"

namespace Switches {
	// CEF and Chromium support a wide range of command-line switches. This file
	// only contains command-line switches specific to the cefclient application.
	// View CEF/Chromium documentation or search for *_switches.cc files in the
	// Chromium source code to identify other existing command-line switches.
	// Below is a partial listing of relevant *_switches.cc files:
	//   base/base_switches.cc
	//   cef/libcef/common/cef_switches.cc
	//   chrome/common/chrome_switches.cc (not all apply)
	//   content/public/common/content_switches.cc

	#if defined(OS_WIN)
	const char kPathSep = '\\';
	#else
	const char kPathSep = '/';
	#endif

	const char kMultiThreadedMessageLoop[] = "multi-threaded-message-loop";
	const char kUrl[] = "url";
	const char kDebug[] = "debug";
	const char kWidth[] = "width";
	const char kHeight[] = "height";
	const char kLocale[] = "locale";
	const char kAppV8Fn[] = "app-v8fn";
	const char kAppPath[] = "app-path";
	const char kCachePath[] = "cache-path";
	const char kRemoteDebugingPort[] = "remote-debugging-port";
	const char kOffScreenRenderingEnabled[] = "off-screen-rendering-enabled";
	const char kOffScreenFrameRate[] = "off-screen-frame-rate";
	const char kTransparentPaintingEnabled[] = "transparent-painting-enabled";
	const char kShowUpdateRect[] = "show-update-rect";
	const char kMouseCursorChangeDisabled[] = "mouse-cursor-change-disabled";
	const char kRequestContextPerBrowser[] = "request-context-per-browser";
	const char kRequestContextSharedCache[] = "request-context-shared-cache";
	const char kBackgroundColor[] = "background-color";
	const char kEnableGPU[] = "enable-gpu";
	const char kFilterURL[] = "filter-url";
	const char kSingleProcess[] = "single_process";
	const char kExternalMessagePump[] = "external-message-pump";
	const char kSharedTextureEnabled[] = "shared-texture-enabled";
	const char kExternalBeginFrameEnabled[] = "external-begin-frame-enabled";
	const char kRequestContextBlockCookies[] = "request-context-block-cookies";
	const char kUseViews[] = "use-views";
	const char kHideFrame[] = "hide-frame";
	const char kAlwaysOnTop[] = "always-on-top";
	const char kShowTopMenu[] = "show-top-menu";
	const char kShowControls[] = "show-controls";
	const char kWidevineCdmPath[] = "widevine-cdm-path";
	const char kSslClientCertificate[] = "ssl-client-certificate";
	const char kCRLSetsPath[] = "crl-sets-path";
	const char kLoadExtension[] = "load-extension";
	const char kLogFile[] = "log-file";
	const char kLogSeverity[] = "log-severity";
	const char kOffline[] = "offline";
	const char kNoActivate[] = "no-activate";
	const char kEnableChromeRuntime[] = "enable-chrome-runtime";
}
