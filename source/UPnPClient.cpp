// UPnPClient.cpp
// UPnP port forwarding for the RocketPlugin plugin.
//
// Author:       Stanbroek
// Version:      0.6.4 24/12/20
//
// References:
//  https://tools.ietf.org/html/rfc6970
//  http://upnp.org/specs/gw/UPnP-gw-WANPPPConnection-v1-Service.pdf
//  http://upnp.org/specs/gw/UPnP-gw-WANIPConnection-v1-Service.pdf
//  http://upnp.org/specs/gw/UPnP-gw-WANIPConnection-v2-Service.pdf

#include "Networking.h"

#include <UPnP.h>
//#include <iphlpapi.h>
//#pragma comment(lib, "iphlpapi.lib")

#include "utils/win32_error_category.h"

#define TO_VARIANT(var, type, valType) \
    VARIANT vt_##var; \
    VariantInit(&vt_##var); \
    vt_##var##.vt = ##type; \
    vt_##var.##valType = ##var

#define MAX_DEVICE_CHILD_DEPTH 16
#define UPNP_WAN_COMMON_INTERFACE_CONFIG L"urn:schemas-upnp-org:service:WANCommonInterfaceConfig:"
#define UPNP_WAN_PPP_CONNECTION          L"urn:schemas-upnp-org:service:WANPPPConnection:"
#define UPNP_WAN_IP_CONNECTION           L"urn:schemas-upnp-org:service:WANIPConnection:"
#define UPNP_TYPE_SIZE(UPNP_TYPE) (sizeof (UPNP_TYPE) - sizeof (UPNP_TYPE)[0])
#define COMPARE(BSTR1, BSTR2)     (memcmp((BSTR1), (BSTR2), UPNP_TYPE_SIZE(BSTR2)) == 0)

/* Additional UPnP error codes. */
#define UPNP_E_SPECIFIED_ARRAY_INDEX_INVALID  MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x0371)
#define UPNP_E_CONFLICT_IN_MAPPING_ENTRY      MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x0376)
#define UPNP_E_SAME_PORT_VALUES_REQUIRED      MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x037C)
#define UPNP_E_ONLY_PERMANENT_LEASE_SUPPORTED MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x037D)


/// <summary>Returns printable representation of the given error.</summary>
/// <param name="hResult">Error code</param>
/// <returns>String representation of the error code</returns>
std::string FormatHResult(const HRESULT hResult)
{
    switch (hResult) {
        case UPNP_E_CONFLICT_IN_MAPPING_ENTRY:
            return "The port mapping entry specified conflicts with a mapping assigned previously to another client.";
        case UPNP_E_SAME_PORT_VALUES_REQUIRED:
            return "Internal and External port values MUST be the same.";
        case UPNP_E_ONLY_PERMANENT_LEASE_SUPPORTED:
            return "The NAT implementation only supports permanent lease times on port mappings.";
        default:
            break;
    }

    char errorBuf[1024];
    DWORD length = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, nullptr,
        hResult, 0, errorBuf, sizeof errorBuf, nullptr);
    if (length == 0) {
        return "Unknown error: (" + to_hex(hResult) + ", " + to_hex(GetLastError()) + ")";
    }
    // trim trailing newline
    while (length > 0 && (errorBuf[length - 1] == '\r' || errorBuf[length - 1] == '\n')) {
        --length;
    }

    return std::string(errorBuf, length) + " (" + to_hex(hResult) + ")";
}


/// <summary>Traverses the service collection and checks if it has a UPnP compatible service.</summary>
/// <remarks>When a UPnP compatible service is found it saves pointers to a
/// couple of child services that should allow for port forwarding.</remarks>
/// <param name="pusServices">Service collection</param>
/// <param name="gtoServices"></param>
/// <param name="gtoFound"></param>
/// <param name="saveServices">Whether it should look for the port forward services</param>
/// <returns>Error code</returns>
HRESULT TraverseServiceCollection(IUPnPServices* pusServices, IUPnPService* gtoServices[2], bool* gtoFound,
                                  const bool saveServices = false)
{
    bool shouldRelease = true;
    IUnknown* pUnk = nullptr;
    HRESULT hr = pusServices->get__NewEnum(&pUnk);
    if (SUCCEEDED(hr)) {
        IEnumVARIANT* pEnumVar = nullptr;
        hr = pUnk->QueryInterface(IID_IEnumVARIANT, reinterpret_cast<void**>(&pEnumVar));
        if (SUCCEEDED(hr)) {
            VARIANT varCurService;
            VariantInit(&varCurService);
            pEnumVar->Reset();
            // Loop through each Service in the collection
            while (S_OK == pEnumVar->Next(1, &varCurService, nullptr)) {
                IUPnPService* pService = nullptr;
                IDispatch* pDispatchDevice = V_DISPATCH(&varCurService);
                hr = pDispatchDevice->QueryInterface(IID_IUPnPService, reinterpret_cast<void**>(&pService));
                if (SUCCEEDED(hr)) {
                    BSTR bstrServiceTypeIdentifier = nullptr;
                    if (SUCCEEDED(pService->get_ServiceTypeIdentifier(&bstrServiceTypeIdentifier))) {
                        if (saveServices) {
                            if (COMPARE(bstrServiceTypeIdentifier, UPNP_WAN_IP_CONNECTION)) {
                                TRACE_LOG("Found UPNP_WAN_IP_CONNECTION");
                                if (gtoServices[0] == nullptr) {
                                    gtoServices[0] = pService;
                                }
                                else {
                                    gtoServices[1] = pService;
                                }
                                shouldRelease = false;
                            }
                            if (COMPARE(bstrServiceTypeIdentifier, UPNP_WAN_PPP_CONNECTION)) {
                                TRACE_LOG("Found UPNP_WAN_PPP_CONNECTION");
                                if (gtoServices[0] == nullptr) {
                                    gtoServices[0] = pService;
                                }
                                else {
                                    gtoServices[1] = pService;
                                }
                                shouldRelease = false;
                            }
                        }
                        else {
                            if (COMPARE(bstrServiceTypeIdentifier, UPNP_WAN_COMMON_INTERFACE_CONFIG)) {
                                TRACE_LOG("Found UPNP_WAN_COMMON_INTERFACE_CONFIG");
                                *gtoFound = true;
                            }
                        }

                        SysFreeString(bstrServiceTypeIdentifier);
                    }
                    else {
                        ERROR_LOG("failed to get the service type identifier: " + FormatHResult(hr));
                    }
                }
                else {
                    ERROR_LOG("failed to query UPnP service: " + FormatHResult(hr));
                }
                VariantClear(&varCurService);
                if (shouldRelease) {
                    pService->Release();
                }
            }
            pEnumVar->Release();

        }
        else {
            ERROR_LOG("failed to query enum variant: " + FormatHResult(hr));
        }
        pUnk->Release();
    }
    else {
        ERROR_LOG("failed to receives a reference to the enumerator interface: " + FormatHResult(hr));
    }

    return hr;
}


/// <summary>Traverses the device collection and checks if it has a UPnP compatible service.</summary>
/// <param name="pDevices">Device collection</param>
/// <param name="gtoDevice"></param>
/// <param name="gtoServices"></param>
/// <param name="depth">Depth of the search</param>
/// <param name="saveDevice">Whether it should look for the port forward services</param>
/// <returns>Error code</returns>
HRESULT TraverseDeviceCollection(IUPnPDevices* pDevices, IUPnPDevice** gtoDevice, IUPnPService* gtoServices[2],
                                 unsigned int depth = 0, bool saveDevice = false)
{
    if (depth >= MAX_DEVICE_CHILD_DEPTH) {
        WARNING_LOG("reached max device child depth");
        return S_OK;
    }

    bool gtoFound = false;
    IUnknown* pUnk = nullptr;
    HRESULT hr = pDevices->get__NewEnum(&pUnk);
    if (SUCCEEDED(hr)) {
        IEnumVARIANT* pEnumVar = nullptr;
        hr = pUnk->QueryInterface(IID_IEnumVARIANT, reinterpret_cast<void**>(&pEnumVar));
        if (SUCCEEDED(hr)) {
            VARIANT varCurDevice;
            VariantInit(&varCurDevice);
            pEnumVar->Reset();
            // Loop through each device in the collection
            while (S_OK == pEnumVar->Next(1, &varCurDevice, nullptr)) {
                IUPnPDevice* pDevice = nullptr;
                IDispatch* pDispatchDevice = V_DISPATCH(&varCurDevice);
                hr = pDispatchDevice->QueryInterface(IID_IUPnPDevice, reinterpret_cast<void**>(&pDevice));
                if (SUCCEEDED(hr)) {
                    if (depth == 0) {
                        VARIANT_BOOL varbIsRoot = VARIANT_FALSE;
                        hr = pDevice->get_IsRootDevice(&varbIsRoot);
                        if (SUCCEEDED(hr)) {
                            if (varbIsRoot == VARIANT_FALSE) {
                                IUPnPDevice* pParentDevice = nullptr;
                                hr = pDevice->get_ParentDevice(&pParentDevice);
                                if (SUCCEEDED(hr)) {
                                    IUPnPServices* pusParentsServices = nullptr;
                                    hr = pParentDevice->get_Services(&pusParentsServices);
                                    if (SUCCEEDED(hr)) {
                                        TraverseServiceCollection(pusParentsServices, gtoServices, &gtoFound);
                                    }
                                    else {
                                        ERROR_LOG("failed to get parent device services: " + FormatHResult(hr));
                                    }
                                    if (gtoFound) {
                                        saveDevice = true;
                                    }

                                    pParentDevice->Release();
                                }
                                else {
                                    ERROR_LOG("failed to get parent device:" + FormatHResult(hr));
                                }
                            }
                        }
                        else {
                            ERROR_LOG("failed to get if device is root device: " + FormatHResult(hr));
                        }
                    }

                    IUPnPServices* pusServices = nullptr;
                    hr = pDevice->get_Services(&pusServices);
                    if (SUCCEEDED(hr)) {
                        hr = TraverseServiceCollection(pusServices, gtoServices, &gtoFound, saveDevice);
                    }
                    else {
                        ERROR_LOG("failed to get services: " + FormatHResult(hr));
                    }

                    if (depth != 0 || gtoFound) {
                        VARIANT_BOOL varbHasChildren = VARIANT_FALSE;
                        hr = pDevice->get_HasChildren(&varbHasChildren);
                        if (SUCCEEDED(hr)) {
                            if (varbHasChildren == VARIANT_TRUE) {
                                IUPnPDevices* pDevicesChildren = nullptr;
                                hr = pDevice->get_Children(&pDevicesChildren);
                                if (SUCCEEDED(hr)) {
                                    hr = TraverseDeviceCollection(pDevicesChildren, gtoDevice, gtoServices, ++depth,
                                        gtoFound);
                                }
                                else {
                                    ERROR_LOG("failed to get children: " + FormatHResult(hr));
                                }
                            }
                        }
                        else {
                            ERROR_LOG("failed to get if device has children: " + FormatHResult(hr));
                        }
                    }
                }
                else {
                    ERROR_LOG("failed to query UPnP device: " + FormatHResult(hr));
                }
                VariantClear(&varCurDevice);
                if (!saveDevice) {
                    pDevice->Release();
                }
                else {
                    *gtoDevice = pDevice;
                    break;
                }
            }
            pEnumVar->Release();
        }
        else {
            ERROR_LOG("failed to query enum variant: " + FormatHResult(hr));
        }
        pUnk->Release();
    }
    else {
        ERROR_LOG("failed to receives a reference to the enumerator interface: " + FormatHResult(hr));
    }

    return hr;
}


/// <summary>Find UPnP devices on the network.</summary>
/// <param name="typeUri">UPnP devices type</param>
void UPnPClient::discoverDevices(const std::wstring& typeUri)
{
    BSTR typeUriBStr = SysAllocString(typeUri.c_str());
    IUPnPDevices* pDevices = nullptr;
    IUPnPDeviceFinder* pUPnPDeviceFinder = nullptr;
    HRESULT hResult = CoCreateInstance(CLSID_UPnPDeviceFinder, nullptr, CLSCTX_INPROC_SERVER,
        IID_IUPnPDeviceFinder, reinterpret_cast<void**>(&pUPnPDeviceFinder));
    if (FAILED(hResult)) {
        ERROR_LOG("failed to create UPnP device finder: " + FormatHResult(hResult));
        return;
    }

    hResult = pUPnPDeviceFinder->FindByType(typeUriBStr, NULL, &pDevices);
    if (SUCCEEDED(hResult)) {
        long lCount = 0;
        hResult = pDevices->get_Count(&lCount);
        if (SUCCEEDED(hResult)) {
            hResult = TraverseDeviceCollection(pDevices, &gtoDevice, gtoServices);
        }
        else {
            ERROR_LOG("failed get device count: " + FormatHResult(hResult));
        }
    }
    else {
        ERROR_LOG("failed to find " + quote(to_string(typeUri)) + " + devices: " + FormatHResult(hResult));
    }
    pUPnPDeviceFinder->Release();
    SysFreeString(typeUriBStr);

    discoveryResult = make_win32_error_code(hResult);
}


/// <summary>Invokes the GetGenericPortMappingEntry action from the UPnP device.</summary>
/// <param name="gtoService">UPnP service</param>
/// <param name="returnStatus">Return value from the service</param>
/// <param name="inPortMappingNumberOfEntries">Number of port mapping entries</param>
/// <param name="outRemoteHost">Remote host</param>
/// <param name="outExternalPort">External port</param>
/// <param name="outPortMappingProtocol">Port mapping protocol</param>
/// <param name="outInternalPort">Internal port</param>
/// <param name="outInternalClient">Internal client</param>
/// <param name="outPortMappingEnabled">Port mapping enabled</param>
/// <param name="outPortMappingDescription">Port mapping description</param>
/// <param name="outPortMappingLeaseDuration">Port mapping lease duration in seconds</param>
/// <returns>Error code</returns>
HRESULT GetGenericPortMappingEntry(IUPnPService* gtoService, std::string& returnStatus,
                                   USHORT inPortMappingNumberOfEntries, BSTR* outRemoteHost, USHORT* outExternalPort,
                                   BSTR* outPortMappingProtocol, USHORT* outInternalPort, BSTR* outInternalClient,
                                   VARIANT_BOOL* outPortMappingEnabled, BSTR* outPortMappingDescription,
                                   ULONG* outPortMappingLeaseDuration)
{
    HRESULT        hr             = S_OK;
    BSTR           bstrActionName = SysAllocString(L"GetGenericPortMappingEntry");
    SAFEARRAYBOUND inArgsBound[1];
    SAFEARRAY*     psaInArgs    = nullptr;
    SAFEARRAY*     psaOutArgs   = nullptr;
    LONG           rgIndices[1] = {0};
    VARIANT        varInArgs;
    VARIANT        varOutArgs;
    VARIANT        varRet;
    VARIANT        varTemp;

    VariantInit(&varInArgs);
    VariantInit(&varOutArgs);
    VariantInit(&varRet);
    VariantInit(&varTemp);

    // Build the arguments to pass to the function.

    TO_VARIANT(inPortMappingNumberOfEntries, VT_UI2, uiVal);

    inArgsBound[0].lLbound = 0;
    inArgsBound[0].cElements = 1;
    psaInArgs = SafeArrayCreate(VT_VARIANT, 1, inArgsBound);
    if (psaInArgs == nullptr) {
        hr = E_OUTOFMEMORY;
    }
    else {
        rgIndices[0] = 0;
        hr = SafeArrayPutElement(psaInArgs, rgIndices, &vt_inPortMappingNumberOfEntries);

        if (SUCCEEDED(hr)) {
            varInArgs.vt = VT_VARIANT | VT_ARRAY;
            V_ARRAY(&varInArgs) = psaInArgs;
        }
    }

    // Invoke the function.

    if (SUCCEEDED(hr)) {
        hr = gtoService->InvokeAction(bstrActionName, varInArgs, &varOutArgs, &varRet);

        if (varRet.vt == VT_BSTR) {
            returnStatus = to_string(varRet.bstrVal);
            TRACE_LOG("returned " + quote(returnStatus));
        }
        else if (varRet.vt != VT_EMPTY) {
            TRACE_LOG("returned " + to_hex(varRet.vt));
        }
    }

    // Retrieve the out arguments from the function.

    if (SUCCEEDED(hr)) {
        psaOutArgs = V_ARRAY(&varOutArgs);

        rgIndices[0] = 0;
        hr = SafeArrayGetElement(psaOutArgs, rgIndices, static_cast<void*>(&varTemp));

        if (SUCCEEDED(hr) && varTemp.vt == VT_BSTR) {
            *outRemoteHost = varTemp.bstrVal;
        }

        if (SUCCEEDED(hr)) {
            rgIndices[0] = 1;
            hr = SafeArrayGetElement(psaOutArgs, rgIndices, static_cast<void*>(&varTemp));
            if (SUCCEEDED(hr) && varTemp.vt == VT_UI2) {
                *outExternalPort = varTemp.uiVal;
            }
        }

        if (SUCCEEDED(hr)) {
            rgIndices[0] = 2;
            hr = SafeArrayGetElement(psaOutArgs, rgIndices, static_cast<void*>(&varTemp));
            if (SUCCEEDED(hr) && varTemp.vt == VT_BSTR) {
                *outPortMappingProtocol = varTemp.bstrVal;
            }
        }

        if (SUCCEEDED(hr)) {
            rgIndices[0] = 3;
            hr = SafeArrayGetElement(psaOutArgs, rgIndices, static_cast<void*>(&varTemp));
            if (SUCCEEDED(hr) && varTemp.vt == VT_UI2) {
                *outInternalPort = varTemp.uiVal;
            }
        }

        if (SUCCEEDED(hr)) {
            rgIndices[0] = 4;
            hr = SafeArrayGetElement(psaOutArgs, rgIndices, static_cast<void*>(&varTemp));
            if (SUCCEEDED(hr) && varTemp.vt == VT_BSTR) {
                *outInternalClient = varTemp.bstrVal;
            }
        }

        if (SUCCEEDED(hr)) {
            rgIndices[0] = 5;
            hr = SafeArrayGetElement(psaOutArgs, rgIndices, static_cast<void*>(&varTemp));
            if (SUCCEEDED(hr) && varTemp.vt == VT_BOOL) {
                *outPortMappingEnabled = varTemp.boolVal;
            }
        }

        if (SUCCEEDED(hr)) {
            rgIndices[0] = 6;
            hr = SafeArrayGetElement(psaOutArgs, rgIndices, static_cast<void*>(&varTemp));
            if (SUCCEEDED(hr) && varTemp.vt == VT_BSTR) {
                *outPortMappingDescription = varTemp.bstrVal;
            }
        }

        if (SUCCEEDED(hr)) {
            rgIndices[0] = 7;
            hr = SafeArrayGetElement(psaOutArgs, rgIndices, static_cast<void*>(&varTemp));
            if (SUCCEEDED(hr) && varTemp.vt == VT_UI4) {
                *outPortMappingLeaseDuration = varTemp.ulVal;
            }
        }
    }

    // Cleanup.

    VariantClear(&vt_inPortMappingNumberOfEntries);

    VariantClear(&varRet);
    VariantClear(&varTemp);

    if (psaInArgs != nullptr) {
        SafeArrayDestroy(psaInArgs);
    }

    if (psaOutArgs != nullptr) {
        SafeArrayDestroy(psaOutArgs);
    }

    SysFreeString(bstrActionName);

    return hr;
}


/// <summary>Invokes the GetSpecificPortMappingEntry action from the UPnP device.</summary>
/// <param name="gtoService">UPnP service</param>
/// <param name="returnStatus">Return value from the service</param>
/// <param name="inRemoteHost">Remote host</param>
/// <param name="inExternalPort">External port</param>
/// <param name="inPortMappingProtocol">Port mapping protocol</param>
/// <param name="outInternalPort">Internal port</param>
/// <param name="outInternalClient">Internal client</param>
/// <param name="outPortMappingEnabled">Port mapping enabled</param>
/// <param name="outPortMappingDescription">Port mapping description</param>
/// <param name="outPortMappingLeaseDuration">Port mapping lease duration in seconds</param>
/// <returns>Error code</returns>
HRESULT GetSpecificPortMappingEntry(IUPnPService* gtoService, std::string& returnStatus, BSTR inRemoteHost,
                                    USHORT inExternalPort, BSTR inPortMappingProtocol, USHORT* outInternalPort,
                                    BSTR* outInternalClient, VARIANT_BOOL* outPortMappingEnabled,
                                    BSTR* outPortMappingDescription, ULONG* outPortMappingLeaseDuration)
{
    HRESULT        hr             = S_OK;
    BSTR           bstrActionName = SysAllocString(L"GetSpecificPortMappingEntry");
    SAFEARRAYBOUND inArgsBound[1];
    SAFEARRAY*     psaInArgs    = nullptr;
    SAFEARRAY*     psaOutArgs   = nullptr;
    LONG           rgIndices[1] = {0};
    VARIANT        varInArgs;
    VARIANT        varOutArgs;
    VARIANT        varRet;
    VARIANT        varTemp;

    VariantInit(&varInArgs);
    VariantInit(&varOutArgs);
    VariantInit(&varRet);
    VariantInit(&varTemp);

    // Build the arguments to pass to the function.

    TO_VARIANT(inRemoteHost, VT_BSTR, bstrVal);
    TO_VARIANT(inExternalPort, VT_UI2, uiVal);
    TO_VARIANT(inPortMappingProtocol, VT_BSTR, bstrVal);

    inArgsBound[0].lLbound = 0;
    inArgsBound[0].cElements = 3;
    psaInArgs = SafeArrayCreate(VT_VARIANT, 1, inArgsBound);
    if (psaInArgs == nullptr) {
        hr = E_OUTOFMEMORY;
    }
    else {
        rgIndices[0] = 0;
        hr = SafeArrayPutElement(psaInArgs, rgIndices, &vt_inRemoteHost);

        if (SUCCEEDED(hr)) {
            rgIndices[0] = 1;
            hr = SafeArrayPutElement(psaInArgs, rgIndices, &vt_inExternalPort);
        }

        if (SUCCEEDED(hr)) {
            rgIndices[0] = 2;
            hr = SafeArrayPutElement(psaInArgs, rgIndices, &vt_inPortMappingProtocol);
        }

        if (SUCCEEDED(hr)) {
            varInArgs.vt = VT_VARIANT | VT_ARRAY;
            V_ARRAY(&varInArgs) = psaInArgs;
        }
    }

    // Invoke the function.

    if (SUCCEEDED(hr)) {
        hr = gtoService->InvokeAction(bstrActionName, varInArgs, &varOutArgs, &varRet);

        if (varRet.vt == VT_BSTR) {
            returnStatus = to_string(varRet.bstrVal);
            TRACE_LOG("returned " + quote(returnStatus));
        }
        else if (varRet.vt != VT_EMPTY) {
            TRACE_LOG("returned " + to_hex(varRet.vt));
        }
    }

    // Retrieve the out arguments from the function.

    if (SUCCEEDED(hr)) {
        psaOutArgs = V_ARRAY(&varOutArgs);

        rgIndices[0] = 0;
        hr = SafeArrayGetElement(psaOutArgs, rgIndices, static_cast<void*>(&varTemp));
        if (SUCCEEDED(hr)) {
            *outInternalPort = varTemp.uiVal;
        }

        if (SUCCEEDED(hr)) {
            rgIndices[0] = 1;
            hr = SafeArrayGetElement(psaOutArgs, rgIndices, static_cast<void*>(&varTemp));
            if (SUCCEEDED(hr)) {
                *outInternalClient = varTemp.bstrVal;
            }
        }

        if (SUCCEEDED(hr)) {
            rgIndices[0] = 2;
            hr = SafeArrayGetElement(psaOutArgs, rgIndices, static_cast<void*>(&varTemp));
            if (SUCCEEDED(hr)) {
                *outPortMappingEnabled = varTemp.boolVal;
            }
        }

        if (SUCCEEDED(hr)) {
            rgIndices[0] = 3;
            hr = SafeArrayGetElement(psaOutArgs, rgIndices, static_cast<void*>(&varTemp));
            if (SUCCEEDED(hr)) {
                *outPortMappingDescription = varTemp.bstrVal;
            }
        }

        if (SUCCEEDED(hr)) {
            rgIndices[0] = 4;
            hr = SafeArrayGetElement(psaOutArgs, rgIndices, static_cast<void*>(&varTemp));
            if (SUCCEEDED(hr)) {
                *outPortMappingLeaseDuration = varTemp.ulVal;
            }
        }
    }

    // Cleanup.

    VariantClear(&vt_inRemoteHost);
    VariantClear(&vt_inExternalPort);
    VariantClear(&vt_inPortMappingProtocol);

    VariantClear(&varRet);
    VariantClear(&varTemp);

    if (psaInArgs != nullptr) {
        SafeArrayDestroy(psaInArgs);
    }

    if (psaOutArgs != nullptr) {
        SafeArrayDestroy(psaOutArgs);
    }

    SysFreeString(bstrActionName);

    return hr;
}


/// <summary>Invokes the AddPortMapping action from the UPnP device.</summary>
/// <param name="gtoService">UPnP service</param>
/// <param name="returnStatus">Return value from the service</param>
/// <param name="inRemoteHost">Remote host</param>
/// <param name="inExternalPort">External port</param>
/// <param name="inPortMappingProtocol">Port mapping protocol</param>
/// <param name="inInternalPort">Internal port</param>
/// <param name="inInternalClient">Internal client</param>
/// <param name="inPortMappingEnabled">Port mapping enabled</param>
/// <param name="inPortMappingDescription">Port mapping description</param>
/// <param name="inPortMappingLeaseDuration">Port mapping lease duration in seconds</param>
/// <returns>Error code</returns>
HRESULT AddPortMapping(IUPnPService* gtoService, std::string& returnStatus, BSTR inRemoteHost, USHORT inExternalPort,
                       BSTR inPortMappingProtocol, USHORT inInternalPort, BSTR inInternalClient,
                       VARIANT_BOOL inPortMappingEnabled, BSTR inPortMappingDescription,
                       ULONG inPortMappingLeaseDuration)
{
    HRESULT hr          = S_OK;
    BSTR bstrActionName = SysAllocString(L"AddPortMapping");
    SAFEARRAYBOUND inArgsBound[1];
    SAFEARRAY*     psaInArgs    = nullptr;
    SAFEARRAY*     psaOutArgs   = nullptr;
    LONG           rgIndices[1] = {0};
    VARIANT        varInArgs;
    VARIANT        varOutArgs;
    VARIANT        varRet;
    VARIANT        varTemp;

    VariantInit(&varInArgs);
    VariantInit(&varOutArgs);
    VariantInit(&varRet);
    VariantInit(&varTemp);

    // Build the arguments to pass to the function.

    TO_VARIANT(inRemoteHost, VT_BSTR, bstrVal);
    TO_VARIANT(inExternalPort, VT_UI2, uiVal);
    TO_VARIANT(inPortMappingProtocol, VT_BSTR, bstrVal);
    TO_VARIANT(inInternalPort, VT_UI2, uiVal);
    TO_VARIANT(inInternalClient, VT_BSTR, bstrVal);
    TO_VARIANT(inPortMappingEnabled, VT_BOOL, boolVal);
    TO_VARIANT(inPortMappingDescription, VT_BSTR, bstrVal);
    TO_VARIANT(inPortMappingLeaseDuration, VT_UI4, ulVal);

    inArgsBound[0].lLbound = 0;
    inArgsBound[0].cElements = 8;
    psaInArgs = SafeArrayCreate(VT_VARIANT, 1, inArgsBound);
    if (psaInArgs == nullptr) {
        hr = E_OUTOFMEMORY;
    }
    else {
        rgIndices[0] = 0;
        hr = SafeArrayPutElement(psaInArgs, rgIndices, &vt_inRemoteHost);

        if (SUCCEEDED(hr)) {
            rgIndices[0] = 1;
            hr = SafeArrayPutElement(psaInArgs, rgIndices, &vt_inExternalPort);
        }

        if (SUCCEEDED(hr)) {
            rgIndices[0] = 2;
            hr = SafeArrayPutElement(psaInArgs, rgIndices, &vt_inPortMappingProtocol);
        }

        if (SUCCEEDED(hr)) {
            rgIndices[0] = 3;
            hr = SafeArrayPutElement(psaInArgs, rgIndices, &vt_inInternalPort);
        }

        if (SUCCEEDED(hr)) {
            rgIndices[0] = 4;
            hr = SafeArrayPutElement(psaInArgs, rgIndices, &vt_inInternalClient);
        }

        if (SUCCEEDED(hr)) {
            rgIndices[0] = 5;
            hr = SafeArrayPutElement(psaInArgs, rgIndices, &vt_inPortMappingEnabled);
        }

        if (SUCCEEDED(hr)) {
            rgIndices[0] = 6;
            hr = SafeArrayPutElement(psaInArgs, rgIndices, &vt_inPortMappingDescription);
        }

        if (SUCCEEDED(hr)) {
            rgIndices[0] = 7;
            hr = SafeArrayPutElement(psaInArgs, rgIndices, &vt_inPortMappingLeaseDuration);
        }

        if (SUCCEEDED(hr)) {
            varInArgs.vt = VT_VARIANT | VT_ARRAY;
            V_ARRAY(&varInArgs) = psaInArgs;
        }
    }

    // Invoke the function.

    if (SUCCEEDED(hr)) {
        hr = gtoService->InvokeAction(bstrActionName, varInArgs, &varOutArgs, &varRet);

        if (varRet.vt == VT_BSTR) {
            returnStatus = to_string(varRet.bstrVal);
            TRACE_LOG("returned " + quote(returnStatus));
        }
        else if (varRet.vt != VT_EMPTY) {
            TRACE_LOG("returned " + to_hex(varRet.vt));
        }
    }

    // Retrieve the out arguments from the function.

    if (SUCCEEDED(hr)) {
        psaOutArgs = V_ARRAY(&varOutArgs);
    }

    // Cleanup.

    VariantClear(&vt_inRemoteHost);
    VariantClear(&vt_inExternalPort);
    VariantClear(&vt_inPortMappingProtocol);
    VariantClear(&vt_inInternalPort);
    VariantClear(&vt_inInternalClient);
    VariantClear(&vt_inPortMappingEnabled);
    VariantClear(&vt_inPortMappingDescription);
    VariantClear(&vt_inPortMappingLeaseDuration);

    VariantClear(&varRet);
    VariantClear(&varTemp);

    if (psaInArgs != nullptr) {
        SafeArrayDestroy(psaInArgs);
    }

    if (psaOutArgs != nullptr) {
        SafeArrayDestroy(psaOutArgs);
    }

    SysFreeString(bstrActionName);

    return hr;
}


/// <summary>Invokes the DeletePortMapping action from the UPnP device.</summary>
/// <param name="gtoService">UPnP service</param>
/// <param name="returnStatus">Return value from the service</param>
/// <param name="inRemoteHost">Remote host</param>
/// <param name="inExternalPort">External port</param>
/// <param name="inPortMappingProtocol">Port mapping protocol</param>
/// <returns>Error code</returns>
HRESULT DeletePortMapping(IUPnPService* gtoService, std::string& returnStatus, BSTR inRemoteHost, USHORT inExternalPort,
                          BSTR inPortMappingProtocol)
{
    HRESULT hr          = S_OK;
    BSTR bstrActionName = SysAllocString(L"DeletePortMapping");
    SAFEARRAYBOUND inArgsBound[1];
    SAFEARRAY*     psaInArgs    = nullptr;
    SAFEARRAY*     psaOutArgs   = nullptr;
    LONG           rgIndices[1] = {0};
    VARIANT        varInArgs;
    VARIANT        varOutArgs;
    VARIANT        varRet;
    VARIANT        varTemp;

    VariantInit(&varInArgs);
    VariantInit(&varOutArgs);
    VariantInit(&varRet);
    VariantInit(&varTemp);

    // Build the arguments to pass to the function.

    TO_VARIANT(inRemoteHost, VT_BSTR, bstrVal);
    TO_VARIANT(inExternalPort, VT_UI2, uiVal);
    TO_VARIANT(inPortMappingProtocol, VT_BSTR, bstrVal);

    inArgsBound[0].lLbound = 0;
    inArgsBound[0].cElements = 3;
    psaInArgs = SafeArrayCreate(VT_VARIANT, 1, inArgsBound);
    if (psaInArgs == nullptr) {
        hr = E_OUTOFMEMORY;
    }
    else {
        rgIndices[0] = 0;
        hr = SafeArrayPutElement(psaInArgs, rgIndices, &vt_inRemoteHost);

        if (SUCCEEDED(hr)) {
            rgIndices[0] = 1;
            hr = SafeArrayPutElement(psaInArgs, rgIndices, &vt_inExternalPort);
        }

        if (SUCCEEDED(hr)) {
            rgIndices[0] = 2;
            hr = SafeArrayPutElement(psaInArgs, rgIndices, &vt_inPortMappingProtocol);
        }

        if (SUCCEEDED(hr)) {
            varInArgs.vt = VT_VARIANT | VT_ARRAY;
            V_ARRAY(&varInArgs) = psaInArgs;
        }
    }

    // Invoke the function.

    if (SUCCEEDED(hr)) {
        hr = gtoService->InvokeAction(bstrActionName, varInArgs, &varOutArgs, &varRet);

        if (varRet.vt == VT_BSTR) {
            returnStatus = to_string(varRet.bstrVal);
            TRACE_LOG("returned " + quote(returnStatus));
        }
        else if (varRet.vt != VT_EMPTY) {
            TRACE_LOG("returned " + to_hex(varRet.vt));
        }
    }

    // Retrieve the out arguments from the function.

    if (SUCCEEDED(hr)) {
        psaOutArgs = V_ARRAY(&varOutArgs);
    }

    // Cleanup.

    VariantClear(&vt_inRemoteHost);
    VariantClear(&vt_inExternalPort);
    VariantClear(&vt_inPortMappingProtocol);

    VariantClear(&varRet);
    VariantClear(&varTemp);

    if (psaInArgs != nullptr) {
        SafeArrayDestroy(psaInArgs);
    }

    if (psaOutArgs != nullptr) {
        SafeArrayDestroy(psaOutArgs);
    }

    SysFreeString(bstrActionName);

    return hr;
}


/// <summary>Invokes the GetExternalIPAddress action from the UPnP device.</summary>
/// <param name="gtoService">UPnP service</param>
/// <param name="returnStatus">Return value from the service</param>
/// <param name="outExternalIPAddress">Remote host</param>
/// <returns>Error code</returns>
HRESULT GetExternalIPAddress(IUPnPService* gtoService, std::string& returnStatus, BSTR* outExternalIPAddress)
{
    HRESULT hr          = S_OK;
    BSTR bstrActionName = SysAllocString(L"GetExternalIPAddress");
    SAFEARRAYBOUND inArgsBound[1];
    SAFEARRAY*     psaInArgs    = nullptr;
    SAFEARRAY*     psaOutArgs   = nullptr;
    LONG           rgIndices[1] = {0};
    VARIANT        varInArgs;
    VARIANT        varOutArgs;
    VARIANT        varRet;
    VARIANT        varTemp;

    VariantInit(&varInArgs);
    VariantInit(&varOutArgs);
    VariantInit(&varRet);
    VariantInit(&varTemp);

    // Build the arguments to pass to the function.

    inArgsBound[0].lLbound = 0;
    inArgsBound[0].cElements = 0;
    psaInArgs = SafeArrayCreate(VT_VARIANT, 1, inArgsBound);
    if (psaInArgs == nullptr) {
        hr = E_OUTOFMEMORY;
    }
    else {
        varInArgs.vt = VT_VARIANT | VT_ARRAY;
        V_ARRAY(&varInArgs) = psaInArgs;
    }

    // Invoke the function.

    if (SUCCEEDED(hr)) {
        hr = gtoService->InvokeAction(bstrActionName, varInArgs, &varOutArgs, &varRet);

        if (varRet.vt == VT_BSTR) {
            returnStatus = to_string(varRet.bstrVal);
            TRACE_LOG("returned " + quote(returnStatus));
        }
        else if (varRet.vt != VT_EMPTY) {
            TRACE_LOG("returned " + to_hex(varRet.vt));
        }
    }

    // Retrieve the out arguments from the function.

    if (SUCCEEDED(hr)) {
        psaOutArgs = V_ARRAY(&varOutArgs);
        rgIndices[0] = 0;

        hr = SafeArrayGetElement(psaOutArgs, rgIndices, static_cast<void*>(&varTemp));
        if (SUCCEEDED(hr)) {
            *outExternalIPAddress = varTemp.bstrVal;
        }
    }

    // Cleanup.

    VariantClear(&varRet);
    VariantClear(&varTemp);

    if (psaInArgs != nullptr) {
        SafeArrayDestroy(psaInArgs);
    }

    if (psaOutArgs != nullptr) {
        SafeArrayDestroy(psaOutArgs);
    }

    SysFreeString(bstrActionName);

    return hr;
}


/// <summary>Forwards the given port through the UPnP device.</summary>
/// <param name="internalPort">Internal port</param>
/// <param name="externalPort">External port</param>
/// <param name="portLeaseDuration">Time in seconds for the port to be opened</param>
/// <param name="threaded">Whether the action should be executed on another thread</param>
void UPnPClient::forwardPort(const unsigned short internalPort, const unsigned short externalPort,
                             const unsigned long portLeaseDuration, const bool threaded)
{
    if (threaded) {
        discoverThread->addJob([=]() {
            forwardPort(internalPort, externalPort, portLeaseDuration, false);
        });
        return;
    }

    addPortMappingReturnStatus.clear();
    addPortMappingStatus = ServiceStatus::SERVICE_BUSY;

    BSTR         inRemoteHost               = SysAllocString(L"");
    USHORT       inExternalPort             = externalPort;
    BSTR         inPortMappingProtocol      = SysAllocString(L"UDP");
    USHORT       inInternalPort             = internalPort;
    BSTR         inInternalClient           = SysAllocString(to_wstring(internalIPAddress).c_str());
    VARIANT_BOOL inPortMappingEnabled       = VARIANT_TRUE;
    BSTR         inPortMappingDescription   = SysAllocString(L"RL port forward for local play");
    ULONG        inPortMappingLeaseDuration = portLeaseDuration;
    HRESULT hResult = AddPortMapping(gtoServices[0], addPortMappingReturnStatus, inRemoteHost, inExternalPort,
                                     inPortMappingProtocol, inInternalPort, inInternalClient, inPortMappingEnabled,
                                     inPortMappingDescription, inPortMappingLeaseDuration);
    if (SUCCEEDED(hResult)) {
        USHORT       outInternalPort             = 0;
        BSTR         outInternalClient           = nullptr;
        VARIANT_BOOL outPortMappingEnabled       = VARIANT_FALSE;
        BSTR         outPortMappingDescription   = nullptr;
        ULONG        outPortMappingLeaseDuration = 0;
        hResult = GetSpecificPortMappingEntry(gtoServices[0], deletePortMappingReturnStatus, inRemoteHost,
                                              inExternalPort, inPortMappingProtocol, &outInternalPort,
                                              &outInternalClient, &outPortMappingEnabled, &outPortMappingDescription,
                                              &outPortMappingLeaseDuration);
        if (SUCCEEDED(hResult)) {
            if (outInternalPort == inInternalPort && wcscmp(outInternalClient, inInternalClient) == 0 &&
                outPortMappingEnabled == inPortMappingEnabled) {
                addPortMappingReturnStatus = std::to_string(externalPort) + " for " + internalIPAddress;
                addPortMappingStatus = ServiceStatus::SERVICE_UPDATED_PORT_MAPPING;
                findOpenPorts(false);
            }
            else {
                addPortMappingReturnStatus += " Received port mapping entry differs from requested port mapping entry, " + 
                    externalIPAddress + ":" + std::to_string(inExternalPort) + " forwarded to " +
                    to_string(outInternalClient) + ":" + std::to_string(outInternalPort) + " (" +
                    to_string(outPortMappingDescription) + ") for " + std::to_string(outPortMappingLeaseDuration) +
                    " seconds";
                addPortMappingResult = make_win32_error_code(hResult);
                addPortMappingStatus = ServiceStatus::SERVICE_ERROR;
            }
            SysFreeString(inRemoteHost);
            SysFreeString(inPortMappingProtocol);
            SysFreeString(inInternalClient);
            SysFreeString(inPortMappingDescription);
            SysFreeString(outInternalClient);
            SysFreeString(outPortMappingDescription);

            return;
        }
    }
    SysFreeString(inRemoteHost);
    SysFreeString(inPortMappingProtocol);
    SysFreeString(inInternalClient);
    SysFreeString(inPortMappingDescription);

    addPortMappingResult = make_win32_error_code(hResult);
    addPortMappingStatus = ServiceStatus::SERVICE_ERROR;
}


/// <summary>Closes a forwarded port through the UPnP device.</summary>
/// <param name="externalPort">External port</param>
/// <param name="threaded">Whether the action should be executed on another thread</param>
void UPnPClient::closePort(const unsigned short externalPort, const bool threaded)
{
    if (threaded) {
        discoverThread->addJob([this, externalPort = externalPort]() {
            closePort(externalPort, false);
        });
        return;
    }

    deletePortMappingReturnStatus.clear();
    deletePortMappingStatus = ServiceStatus::SERVICE_BUSY;

    BSTR   inRemoteHost          = SysAllocString(L"");
    USHORT inExternalPort        = externalPort;
    BSTR   inPortMappingProtocol = SysAllocString(L"UDP");
    HRESULT hResult = DeletePortMapping(gtoServices[0], deletePortMappingReturnStatus, inRemoteHost, inExternalPort,
                                        inPortMappingProtocol);
    if (SUCCEEDED(hResult)) {
        deletePortMappingReturnStatus = std::to_string(externalPort) + " for " + internalIPAddress;
        deletePortMappingStatus = ServiceStatus::SERVICE_UPDATED_PORT_MAPPING;
    }
    else {
        deletePortMappingResult = make_win32_error_code(hResult);
        deletePortMappingStatus = ServiceStatus::SERVICE_ERROR;
    }
    findOpenPorts(false);

    SysFreeString(inRemoteHost);
    SysFreeString(inPortMappingProtocol);
}


/// <summary>Find open ports through the UPnP device.</summary>
/// <param name="threaded">Whether the action should be executed on another thread</param>
void UPnPClient::findOpenPorts(const bool threaded)
{
    if (threaded) {
        discoverThread->addJob([this]() {
            findOpenPorts(false);
        });
        return;
    }

    openPorts.clear();
    USHORT inPortMappingNumberOfEntries = 0;
    HRESULT hResult = S_OK;
    while (SUCCEEDED(hResult)) {
        deletePortMappingStatus = ServiceStatus::SERVICE_BUSY;
        BSTR         outRemoteHost               = nullptr;
        USHORT       outExternalPort             = 0;
        BSTR         outPortMappingProtocol      = nullptr;
        USHORT       outInternalPort             = 0;
        BSTR         outInternalClient           = nullptr;
        VARIANT_BOOL outPortMappingEnabled       = VARIANT_FALSE;
        BSTR         outPortMappingDescription   = nullptr;
        ULONG        outPortMappingLeaseDuration = 0;
        hResult = GetGenericPortMappingEntry(gtoServices[0], deletePortMappingReturnStatus,
                                             inPortMappingNumberOfEntries, &outRemoteHost, &outExternalPort,
                                             &outPortMappingProtocol, &outInternalPort, &outInternalClient,
                                             &outPortMappingEnabled, &outPortMappingDescription,
                                             &outPortMappingLeaseDuration);
        if (SUCCEEDED(hResult)) {
            inPortMappingNumberOfEntries++;
            if (to_string(outInternalClient) == internalIPAddress) {
                openPorts.push_back(outExternalPort);
            }
            SysFreeString(outRemoteHost);
            SysFreeString(outPortMappingProtocol);
            SysFreeString(outInternalClient);
            SysFreeString(outPortMappingDescription);
        }
    }

    // We reached the end of the mapped port entries.
    if (hResult == UPNP_E_SPECIFIED_ARRAY_INDEX_INVALID) {
        deletePortMappingStatus = ServiceStatus::SERVICE_FINISHED;
    }
    else {
        deletePortMappingResult = make_win32_error_code(hResult);
        deletePortMappingStatus = ServiceStatus::SERVICE_ERROR;
    }
}


/// <summary>Clears the saved UPnP devices.</summary>
void UPnPClient::clearDevices()
{
    if (gtoServices[0] != nullptr) {
        gtoServices[0]->Release();
        gtoServices[0] = nullptr;
    }
    if (gtoServices[1] != nullptr) {
        gtoServices[1]->Release();
        gtoServices[1] = nullptr;
    }
    if (gtoDevice != nullptr) {
        gtoDevice->Release();
        gtoDevice = nullptr;
    }
}


/// <summary>Find UPnP compatible device on the network.</summary>
/// <param name="threaded">Whether the action should be executed on another thread</param>
void UPnPClient::findDevices(const bool threaded)
{
    if (threaded) {
        discoverThread->addJob([this]() {
            findDevices(false);
        });
        return;
    }

    std::vector<std::wstring> const deviceList = {
        //L"urn:schemas-upnp-org:device:InternetGatewayDevice:2",
        //L"urn:schemas-upnp-org:service:WANIPConnection:2",
        L"urn:schemas-upnp-org:device:InternetGatewayDevice:1",
        L"urn:schemas-upnp-org:service:WANIPConnection:1",
        L"urn:schemas-upnp-org:service:WANPPPConnection:1",
        L"upnp:rootdevice"
        //L"ssdp:all"
    };

    clearDevices();
    addPortMappingStatus = ServiceStatus::SERVICE_IDLE;
    deletePortMappingStatus = ServiceStatus::SERVICE_IDLE;
    discoveryStatus = DiscoveryStatus::DISCOVERY_BUSY;
    for (const std::wstring& device : deviceList) {
        discoverDevices(device);
        if (gtoServices[0] != nullptr) {
            break;
        }
    }

    if (gtoServices[0] != nullptr) {
        addPortMappingStatus = ServiceStatus::SERVICE_BUSY;
        discoveryStatus = DiscoveryStatus::DISCOVERY_FINISHED;
        BSTR bstrExternalIPAddress = nullptr;
        HRESULT hResult = GetExternalIPAddress(gtoServices[0], discoveryReturnStatus, &bstrExternalIPAddress);
        if (SUCCEEDED(hResult)) {
            externalIPAddress = to_string(bstrExternalIPAddress);
            addPortMappingStatus = ServiceStatus::SERVICE_GOT_EXT_IP;
        }
        else {
            // Swap WANPPPConnection and WANIPConnection, so GTOServices[0] always works.
            IUPnPService* tmp = gtoServices[0];
            gtoServices[0] = gtoServices[1];
            gtoServices[1] = tmp;
            hResult = GetExternalIPAddress(gtoServices[0], discoveryReturnStatus, &bstrExternalIPAddress);
            if (SUCCEEDED(hResult)) {
                externalIPAddress = to_string(bstrExternalIPAddress);
                addPortMappingStatus = ServiceStatus::SERVICE_GOT_EXT_IP;
            }
            else {
                discoveryResult = make_win32_error_code(hResult);
                addPortMappingStatus = ServiceStatus::SERVICE_ERROR;
                ERROR_LOG("failed to get external ip address");
            }
        }

        if (SUCCEEDED(hResult)) {
            hResult = Networking::GetInternalIPAddress(internalIPAddress).value();
            if (FAILED(hResult)) {
                addPortMappingResult = make_win32_error_code(hResult);
                addPortMappingStatus = ServiceStatus::SERVICE_ERROR;
            }
            else if (internalIPAddress.empty()) {
                addPortMappingReturnStatus = "Failed to get internal ip address.";
                addPortMappingResult = make_win32_error_code(S_OK);
                addPortMappingStatus = ServiceStatus::SERVICE_ERROR;
                ERROR_LOG("failed to get internal ip address");
            }

            findOpenPorts(false);
        }
    }
    else {
        discoveryStatus = DiscoveryStatus::DISCOVERY_ERROR;
    }
}


/// <summary>Gets the friendly name from the UPnP device.</summary>
/// <param name="pDevice">UPnP device</param>
/// <returns>The friendly name from the UPnP device</returns>
std::string UPnPClient::getDeviceFriendlyName(IUPnPDevice* pDevice)
{
    if (deviceFriendlyName.empty()) {
        BSTR bstrFriendlyName = nullptr;
        const HRESULT hr = pDevice->get_FriendlyName(&bstrFriendlyName);
        if (SUCCEEDED(hr)) {
            deviceFriendlyName = to_string(bstrFriendlyName);
        }
        else {
            deviceFriendlyName = "Unknown Name\"\n\"" + FormatHResult(hr) + "\"";
            ERROR_LOG("failed to get friendly name: " + FormatHResult(hr));
        }
    }

    return deviceFriendlyName;
}


/// <summary>Gets the current discovery status.</summary>
/// <returns>A description of the current discovery status</returns>
std::string UPnPClient::getDiscoveryStatus()
{
    switch (discoveryStatus) {
        case DiscoveryStatus::DISCOVERY_IDLE:
            return "";
        case DiscoveryStatus::DISCOVERY_ERROR:
            if (SUCCEEDED(discoveryResult.value())) {
                return "Could not find any compatible devices. " + discoveryReturnStatus;
            }
            return "Error: " + FormatHResult(discoveryResult.value()) + " " + discoveryReturnStatus;
        case DiscoveryStatus::DISCOVERY_BUSY:
            return "Searching for UPnP compatible devices.";
        case DiscoveryStatus::DISCOVERY_FINISHED:
            return "Found a UPnP compatible device. \"" + getDeviceFriendlyName(gtoDevice) + "\"";
    }

    return "Unknown status";
}


/// <summary>Gets the current port forwarding status.</summary>
/// <returns>A description of the current port forwarding status</returns>
std::string UPnPClient::getForwardPortStatus() const
{
    switch (addPortMappingStatus) {
        case ServiceStatus::SERVICE_IDLE:
        case ServiceStatus::SERVICE_FINISHED:
            return "Waiting for input.";
        case ServiceStatus::SERVICE_ERROR:
            if (FAILED(addPortMappingResult.value())) {
                return "Error: " + FormatHResult(addPortMappingResult.value()) + " " + addPortMappingReturnStatus;
            }
            if (!addPortMappingReturnStatus.empty()) {
                return "Error: " + addPortMappingReturnStatus;
            }
            return "Unknown error: (" + to_hex(addPortMappingResult.value()) + ")";
        case ServiceStatus::SERVICE_BUSY:
            return "Connecting with service.";
        case ServiceStatus::SERVICE_GOT_EXT_IP:
            return "Successfully connected with service as " + internalIPAddress + ".";
        case ServiceStatus::SERVICE_UPDATED_PORT_MAPPING:
            return "Successfully redirected port " + addPortMappingReturnStatus;
    }

    return "Unknown status";
}


/// <summary>Gets the current forwarded port closing status.</summary>
/// <returns>A description of the current forwarded port closing status</returns>
std::string UPnPClient::getClosePortStatus() const
{
    switch (deletePortMappingStatus) {
        case ServiceStatus::SERVICE_IDLE:
        case ServiceStatus::SERVICE_FINISHED:
            return "";
        case ServiceStatus::SERVICE_ERROR:
            if (FAILED(deletePortMappingResult.value())) {
                return "Error: " + FormatHResult(deletePortMappingResult.value()) + " " + deletePortMappingReturnStatus;
            }
            if (!deletePortMappingReturnStatus.empty()) {
                return "Error: " + deletePortMappingReturnStatus;
            }
            return "Unknown error: (" + to_hex(deletePortMappingResult.value()) + ")";
        case ServiceStatus::SERVICE_BUSY:
            return "Connecting with service.";
        case ServiceStatus::SERVICE_GOT_EXT_IP:
            return "Successfully connected with service as " + internalIPAddress + ".";
        case ServiceStatus::SERVICE_UPDATED_PORT_MAPPING:
            return "Successfully closed port " + deletePortMappingReturnStatus;
    }

    return "Unknown status " + deletePortMappingReturnStatus;
}


/// <summary>Initializes the discovery thread <see cref="WorkerThread"/>.</summary>
UPnPClient::UPnPClient()
{
    discoverThread = std::make_unique<WorkerThread>();
}


/// <summary>Clears the saved devices and exits the discovery thread <see cref="WorkerThread"/>.</summary>
UPnPClient::~UPnPClient()
{
    clearDevices();
}
