// UPnPClient.cpp
// UPnP port forwarding for the RocketPlugin plugin.
//
// Author:       Stanbroek
// Version:      0.6.3 15/7/20
//
// References:
//  http://upnp.org/specs/gw/UPnP-gw-WANPPPConnection-v1-Service.pdf
//  http://upnp.org/specs/gw/UPnP-gw-WANIPConnection-v1-Service.pdf
//  http://upnp.org/specs/gw/UPnP-gw-WANIPConnection-v2-Service.pdf
// TODO: links are broken :/
//  https://tools.ietf.org/html/rfc6970

#include "Networking.h"

#include <upnp.h>
#include <iostream>
#include <iphlpapi.h>
#pragma comment(lib, "iphlpapi.lib")

// Just use the UPnP librairy, it would be easy I though...
#define TO_VARIANT(var, type, valtype) \
    VARIANT vt_##var; \
    VariantInit(&vt_##var); \
    vt_##var##.vt = ##type; \
    vt_##var.##valtype = ##var;

#define MAX_DEVICE_CHILD_DEPTH 16
#define UPNP_WAN_COMMON_INTERFACE_CONFIG L"urn:schemas-upnp-org:service:WANCommonInterfaceConfig:"
#define UPNP_WAN_PPP_CONNECTION          L"urn:schemas-upnp-org:service:WANPPPConnection:"
#define UPNP_WAN_IP_CONNECTION           L"urn:schemas-upnp-org:service:WANIPConnection:"
#define UPNP_TYPE_SIZE(UPNP_TYPE) (sizeof UPNP_TYPE - sizeof UPNP_TYPE[0])
#define COMPARE(BSTR1, BSTR2)     (memcmp(BSTR1, BSTR2, UPNP_TYPE_SIZE(BSTR2)) == 0)

/* Additional UPnP error codes. */
#define UPNP_E_SPECIFIED_ARRAY_INDEX_INVALID  MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x0371)
#define UPNP_E_CONFLICT_IN_MAPPING_ENTRY      MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x0376)
#define UPNP_E_SAME_PORT_VALUES_REQUIRED      MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x037C)
#define UPNP_E_ONLY_PERMANENT_LEASE_SUPPORTED MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x037D)


// Using namespace std to keep the style consistent.
namespace std {
    /// <summary>Converts wide char pointer to strings.</summary>
    /// <param name="wcstr">Wide char pointer</param>
    /// <returns>Converted string</returns>
    std::string to_string(const wchar_t* wcstr)
    {
        if (wcstr == nullptr) {
            return "EMPTY STRING!";
        }

        size_t length = std::wcslen(wcstr);
        std::string str(length, '\0');
        std::use_facet<std::ctype<wchar_t>>(std::locale()).narrow(wcstr, wcstr + length, '?', str.data());

        return str;
    }

    /// <summary>Converts wide strings to strings.</summary>
    /// <param name="wstr">Wide string</param>
    /// <returns>Converted string</returns>
    std::string to_string(std::wstring wstr)
    {
        return to_string(wstr.c_str());
    }

    /// <summary>Converts char pointer to wide strings.</summary>
    /// <param name="cstr">char pointer</param>
    /// <returns>Converted wide string</returns>
    std::wstring to_wstring(const char* cstr)
    {
        if (cstr == nullptr) {
            return L"EMPTY STRING!";
        }

        size_t length = std::strlen(cstr);
        std::wstring wstr(length, '\0');
        std::use_facet<std::ctype<wchar_t>>(std::locale()).widen(cstr, cstr + length, wstr.data());

        return wstr;
    }

    /// <summary>Converts strings to wide strings.</summary>
    /// <param name="str">String</param>
    /// <returns>Converted wide string</returns>
    std::wstring to_wstring(std::string str)
    {
        return to_wstring(str.data());
    }
}


/// <summary>Converts an (Ipv4) Internet network address into an ASCII string in Internet standard dotted-decimal format.</summary>
/// <remarks>inet_ntoa from inet_ntoa.c</remarks>
/// <param name="in">Ipv4 Internet network address</param>
/// <returns>network address as ASCII string</returns>
char* inet_ntos(struct in_addr in)
{
    static char buffer[18];
    unsigned char* bytes = (unsigned char*)&in;
    snprintf(buffer, sizeof(buffer), "%d.%d.%d.%d",
        bytes[0], bytes[1], bytes[2], bytes[3]);

    return buffer;
}


/// <summary>Checks if an IP address is a private (LAN) address.</summary>
/// <remarks>From https://tools.ietf.org/html/rfc1918</remarks>
/// <param name="addr">IP address</param>
/// <returns>Bool with if <see cref="addr"/> is a private (LAN) address</returns>
static bool is_rfc1918addr(const char* addr)
{
    // 192.168.0.0     -   192.168.255.255 (192.168/16 prefix)
    if (COMPARE(addr, "192.168.")) {
        return true;
    }
    // 10.0.0.0        -   10.255.255.255  (10/8 prefix)
    if (COMPARE(addr, "10.")) {
        return true;
    }
    // 172.16.0.0      -   172.31.255.255  (172.16/12 prefix)
    if (COMPARE(addr, "172.")) {
        int i = atoi(addr + 4);
        if ((16 <= i) && (i <= 31)) {
            return true;
        }
    }

    return false;
}


/// <summary>Returns printable representation of the given error.</summary>
/// <param name="hr">Error code</param>
/// <returns>Printable representation of <see cref="hr"/></returns>
std::string printHResult(HRESULT hr)
{
    switch (hr)
    {
    case UPNP_E_CONFLICT_IN_MAPPING_ENTRY:
        return "The port mapping entry specified conflicts with a mapping assigned previously to another client.";
    case UPNP_E_SAME_PORT_VALUES_REQUIRED:
        return "Internal and External port values MUST be the same.";
    case UPNP_E_ONLY_PERMANENT_LEASE_SUPPORTED:
        return "The NAT implementation only supports permanent lease times on port mappings.";
    default:
        break;
    }

    return std::system_category().message(hr) + " (" + std::to_string(hr) + ")";
}


/// <summary>Traverses the service collection and checks if it has a UPnP compatible service.</summary>
/// <remarks>When a UPnP compatible service is found it saves pointers to a couple of child services that should allow for port forwarding.</remarks>
/// <param name="pusServices">Service collection</param>
/// <param name="saveServices">Whether it should look for the port forward services</param>
/// <returns>Error code</returns>
HRESULT UPnPClient::TraverseServiceCollection(IUPnPServices* pusServices, bool saveServices)
{
    bool shouldRelease = true;
    IUnknown* pUnk = NULL;
    HRESULT hr = pusServices->get__NewEnum(&pUnk);
    if (SUCCEEDED(hr)) {
        IEnumVARIANT* pEnumVar = NULL;
        hr = pUnk->QueryInterface(IID_IEnumVARIANT, (void**)&pEnumVar);
        if (SUCCEEDED(hr)) {
            VARIANT varCurService;
            VariantInit(&varCurService);
            pEnumVar->Reset();
            // Loop through each Service in the collection
            while (S_OK == pEnumVar->Next(1, &varCurService, NULL)) {
                IUPnPService* pService = NULL;
                IDispatch* pdispDevice = V_DISPATCH(&varCurService);
                hr = pdispDevice->QueryInterface(IID_IUPnPService, (void**)&pService);
                if (SUCCEEDED(hr)) {
                    BSTR bstrServiceTypeIdentifier = NULL;
                    if (SUCCEEDED(pService->get_ServiceTypeIdentifier(&bstrServiceTypeIdentifier))) {
                        if (saveServices) {
                            if (COMPARE(bstrServiceTypeIdentifier, UPNP_WAN_IP_CONNECTION)) {
                                if (GTOServices[0] == nullptr) {
                                    GTOServices[0] = pService;
                                }
                                else {
                                    GTOServices[1] = pService;
                                }
                                shouldRelease = false;
                            }
                            if (COMPARE(bstrServiceTypeIdentifier, UPNP_WAN_PPP_CONNECTION)) {
                                if (GTOServices[0] == nullptr) {
                                    GTOServices[0] = pService;
                                }
                                else {
                                    GTOServices[1] = pService;
                                }
                                shouldRelease = false;
                            }
                        }
                        else {
                            if (COMPARE(bstrServiceTypeIdentifier, UPNP_WAN_COMMON_INTERFACE_CONFIG)) {
                                discoveryStatus = DiscoveryStatus::DISCOVERY_FOUND_GTO;
                            }
                        }

                        SysFreeString(bstrServiceTypeIdentifier);
                    }
                }
                VariantClear(&varCurService);
                if (shouldRelease) {
                    pService->Release();
                }
            }
            pEnumVar->Release();
        }
        pUnk->Release();
    }

    return hr;
}


/// <summary>Traverses the device collection and checks if it has a UPnP compatible service.</summary>
/// <param name="pusServices">Device collection</param>
/// <param name="depth">Depth of the search</param>
/// <param name="saveServices">Whether it should look for the port forward services</param>
/// <returns>Error code</returns>
HRESULT UPnPClient::TraverseDeviceCollection(IUPnPDevices* pDevices, unsigned int depth, bool saveDevice)
{
    if (depth >= MAX_DEVICE_CHILD_DEPTH) {
        return S_OK;
    }

    IUnknown* pUnk = NULL;
    HRESULT hr = pDevices->get__NewEnum(&pUnk);
    if (SUCCEEDED(hr)) {
        IEnumVARIANT* pEnumVar = NULL;
        hr = pUnk->QueryInterface(IID_IEnumVARIANT, (void**)&pEnumVar);
        if (SUCCEEDED(hr)) {
            VARIANT varCurDevice;
            VariantInit(&varCurDevice);
            pEnumVar->Reset();
            // Loop through each device in the collection
            while (S_OK == pEnumVar->Next(1, &varCurDevice, NULL)) {
                IUPnPDevice* pDevice = NULL;
                IDispatch* pdispDevice = V_DISPATCH(&varCurDevice);
                hr = pdispDevice->QueryInterface(IID_IUPnPDevice, (void**)&pDevice);
                if (SUCCEEDED(hr)) {
                    if (depth == 0) {
                        VARIANT_BOOL varbIsRoot = VARIANT_FALSE;
                        hr = pDevice->get_IsRootDevice(&varbIsRoot);
                        if (SUCCEEDED(hr) && varbIsRoot == VARIANT_FALSE) {
                            IUPnPDevice* pParentDevice = NULL;
                            hr = pDevice->get_ParentDevice(&pParentDevice);
                            if (SUCCEEDED(hr)) {
                                IUPnPServices* pusParentsServices = NULL;
                                hr = pParentDevice->get_Services(&pusParentsServices);
                                if (SUCCEEDED(hr)) {
                                    hr = TraverseServiceCollection(pusParentsServices);
                                }
                                if (discoveryStatus == DiscoveryStatus::DISCOVERY_FOUND_GTO) {
                                    saveDevice = true;
                                }

                                pParentDevice->Release();
                            }
                        }
                    }

                    IUPnPServices* pusServices = NULL;
                    hr = pDevice->get_Services(&pusServices);
                    if (SUCCEEDED(hr)) {
                        hr = TraverseServiceCollection(pusServices, saveDevice);
                    }

                    if (depth != 0 || discoveryStatus == DiscoveryStatus::DISCOVERY_BUSY) {
                        VARIANT_BOOL varbHasChildren = VARIANT_FALSE;
                        hr = pDevice->get_HasChildren(&varbHasChildren);
                        if (SUCCEEDED(hr) && varbHasChildren == VARIANT_TRUE) {
                            IUPnPDevices* pDevicesChildren = NULL;
                            hr = pDevice->get_Children(&pDevicesChildren);
                            if (SUCCEEDED(hr)) {
                                hr = TraverseDeviceCollection(pDevicesChildren, ++depth, discoveryStatus == DiscoveryStatus::DISCOVERY_FOUND_GTO);
                            }
                        }
                    }
                }
                VariantClear(&varCurDevice);
                if (!saveDevice) {
                    pDevice->Release();
                }
                else {
                    GTODevice = pDevice;
                    break;
                }
            }
            pEnumVar->Release();
        }
        pUnk->Release();
    }

    return hr;
}


/// <summary>Find UPnP devices on the network.</summary>
/// <param name="TypeUri">UPnP devices type</param>
void UPnPClient::DiscoverDevices(std::wstring TypeUri)
{
    hResult = S_OK;
    BSTR TypeURI = SysAllocString(TypeUri.c_str());
    IUPnPDevices* pDevices = NULL;
    IUPnPDeviceFinder* pUPnPDeviceFinder = NULL;
    hResult = CoCreateInstance(CLSID_UPnPDeviceFinder, NULL, CLSCTX_INPROC_SERVER, IID_IUPnPDeviceFinder, reinterpret_cast<void**>(&pUPnPDeviceFinder));
    if (!SUCCEEDED(hResult)) {
        return;
    }

    hResult = pUPnPDeviceFinder->FindByType(TypeURI, NULL, &pDevices);
    if (SUCCEEDED(hResult)) {
        long lCount = 0;
        hResult = pDevices->get_Count(&lCount);
        if (SUCCEEDED(hResult)) {
            hResult = TraverseDeviceCollection(pDevices);
        }
    }
    pUPnPDeviceFinder->Release();
    SysFreeString(TypeURI);
}


/// <summary>Invokes the GetGenericPortMappingEntry action from the UPnP device.</summary>
/// <param name="GTOService">UPnP service</param>
/// <param name="returnStatus">Return value from the service</param>
/// <param name="PortMappingNumberOfEntries">Number of port mapping entries</param>
/// <param name="RemoteHost">Remote host</param>
/// <param name="ExternalPort">External port</param>
/// <param name="PortMappingProtocol">Port mapping protocol</param>
/// <param name="InternalPort">Internal port</param>
/// <param name="InternalClient">Internal client</param>
/// <param name="PortMappingEnabled">Port mapping enabled</param>
/// <param name="PortMappingDescription">Port mapping description</param>
/// <param name="PortMappingLeaseDuration">Port mapping lease duration in seconds</param>
/// <returns>Error code</returns>
HRESULT GetGenericPortMappingEntry(IUPnPService* GTOService, std::string returnStatus, USHORT PortMappingNumberOfEntries, BSTR* RemoteHost, USHORT* ExternalPort, BSTR* PortMappingProtocol, USHORT* InternalPort, BSTR* InternalClient, VARIANT_BOOL* PortMappingEnabled, BSTR* PortMappingDescription, ULONG* PortMappingLeaseDuration)
{
    HRESULT        hr             = S_OK;
    BSTR           bstrActionName = SysAllocString(L"GetGenericPortMappingEntry");
    SAFEARRAYBOUND inArgsBound[1];
    SAFEARRAY*     psaInArgs      = NULL;
    SAFEARRAY*     psaOutArgs     = NULL;
    LONG           rgIndices[1]   = { 0 };
    VARIANT        varInArgs      = { 0 };
    VARIANT        varOutArgs     = { 0 };
    VARIANT        varRet         = { 0 };
    VARIANT        varTemp        = { 0 };

    VariantInit(&varInArgs);
    VariantInit(&varOutArgs);
    VariantInit(&varRet);
    VariantInit(&varTemp);

    // Build the arguments to pass to the function

    TO_VARIANT(PortMappingNumberOfEntries, VT_UI2, uiVal);

    inArgsBound[0].lLbound = 0;
    inArgsBound[0].cElements = 1;
    psaInArgs = SafeArrayCreate(VT_VARIANT, 1, inArgsBound);
    if (psaInArgs == NULL) {
        hr = E_OUTOFMEMORY;
    }
    else {
        rgIndices[0] = 0;
        hr = SafeArrayPutElement(psaInArgs, rgIndices, &vt_PortMappingNumberOfEntries);

        if (SUCCEEDED(hr)) {
            varInArgs.vt = VT_VARIANT | VT_ARRAY;
            V_ARRAY(&varInArgs) = psaInArgs;
        }
    }

    // Invoke the function

    if (SUCCEEDED(hr)) {
        hr = GTOService->InvokeAction(bstrActionName, varInArgs, &varOutArgs, &varRet);

        if (varRet.vt == VT_BSTR) {
            returnStatus = std::to_string(varRet.bstrVal);
        }
    }

    if (SUCCEEDED(hr)) {
        psaOutArgs = V_ARRAY(&varOutArgs);

        rgIndices[0] = 0;
        hr = SafeArrayGetElement(psaOutArgs, rgIndices, (void*)&varTemp);
        if (SUCCEEDED(hr) && varTemp.vt == VT_BSTR) {
            *RemoteHost = varTemp.bstrVal;
        }
            

        if (SUCCEEDED(hr)) {
            rgIndices[0] = 1;
            hr = SafeArrayGetElement(psaOutArgs, rgIndices, (void*)&varTemp);
            if (SUCCEEDED(hr) && varTemp.vt == VT_UI2) {
                *ExternalPort = varTemp.uiVal;
            }
        }

        if (SUCCEEDED(hr)) {
            rgIndices[0] = 2;
            hr = SafeArrayGetElement(psaOutArgs, rgIndices, (void*)&varTemp);
            if (SUCCEEDED(hr) && varTemp.vt == VT_BSTR) {
                *PortMappingProtocol = varTemp.bstrVal;
            }
        }

        if (SUCCEEDED(hr)) {
            rgIndices[0] = 3;
            hr = SafeArrayGetElement(psaOutArgs, rgIndices, (void*)&varTemp);
            if (SUCCEEDED(hr) && varTemp.vt == VT_UI2) {
                *InternalPort = varTemp.uiVal;
            }
        }

        if (SUCCEEDED(hr)) {
            rgIndices[0] = 4;
            hr = SafeArrayGetElement(psaOutArgs, rgIndices, (void*)&varTemp);
            if (SUCCEEDED(hr) && varTemp.vt == VT_BSTR) {
                *InternalClient = varTemp.bstrVal;
            }
        }

        if (SUCCEEDED(hr)) {
            rgIndices[0] = 5;
            hr = SafeArrayGetElement(psaOutArgs, rgIndices, (void*)&varTemp);
            if (SUCCEEDED(hr) && varTemp.vt == VT_BOOL) {
                *PortMappingEnabled = varTemp.boolVal;
            }
        }

        if (SUCCEEDED(hr)) {
            rgIndices[0] = 6;
            hr = SafeArrayGetElement(psaOutArgs, rgIndices, (void*)&varTemp);
            if (SUCCEEDED(hr) && varTemp.vt == VT_BSTR) {
                *PortMappingDescription = varTemp.bstrVal;
            }
        }

        if (SUCCEEDED(hr)) {
            rgIndices[0] = 7;
            hr = SafeArrayGetElement(psaOutArgs, rgIndices, (void*)&varTemp);
            if (SUCCEEDED(hr) && varTemp.vt == VT_UI4) {
                *PortMappingLeaseDuration = varTemp.ulVal;
            }
        }
    }

    // Cleanup

    VariantClear(&vt_PortMappingNumberOfEntries);

    VariantClear(&varRet);
    VariantClear(&varTemp);

    if (NULL != psaInArgs) {
        SafeArrayDestroy(psaInArgs);
    }

    if (NULL != psaOutArgs) {
        SafeArrayDestroy(psaOutArgs);
    }

    SysFreeString(bstrActionName);

    return hr;
}


/// <summary>Invokes the GetSpecificPortMappingEntry action from the UPnP device.</summary>
/// <param name="GTOService">UPnP service</param>
/// <param name="returnStatus">Return value from the service</param>
/// <param name="RemoteHost">Remote host</param>
/// <param name="ExternalPort">External port</param>
/// <param name="PortMappingProtocol">Port mapping protocol</param>
/// <param name="InternalPort">Internal port</param>
/// <param name="InternalClient">Internal client</param>
/// <param name="PortMappingEnabled">Port mapping enabled</param>
/// <param name="PortMappingDescription">Port mapping description</param>
/// <param name="PortMappingLeaseDuration">Port mapping lease duration in seconds</param>
/// <returns>Error code</returns>
HRESULT GetSpecificPortMappingEntry(IUPnPService* GTOService, std::string returnStatus, BSTR RemoteHost, USHORT ExternalPort, BSTR PortMappingProtocol, USHORT* InternalPort, BSTR* InternalClient, VARIANT_BOOL* PortMappingEnabled, BSTR* PortMappingDescription, ULONG* PortMappingLeaseDuration)
{
    HRESULT        hr             = S_OK;
    BSTR           bstrActionName = SysAllocString(L"GetSpecificPortMappingEntry");
    SAFEARRAYBOUND inArgsBound[1];
    SAFEARRAY*     psaInArgs      = NULL;
    SAFEARRAY*     psaOutArgs     = NULL;
    LONG           rgIndices[1]   = { 0 };
    VARIANT        varInArgs      = { 0 };
    VARIANT        varOutArgs     = { 0 };
    VARIANT        varRet         = { 0 };
    VARIANT        varTemp        = { 0 };

    VariantInit(&varInArgs);
    VariantInit(&varOutArgs);
    VariantInit(&varRet);
    VariantInit(&varTemp);

    // Build the arguments to pass to the function

    TO_VARIANT(RemoteHost, VT_BSTR, bstrVal);
    TO_VARIANT(ExternalPort, VT_UI2, uiVal);
    TO_VARIANT(PortMappingProtocol, VT_BSTR, bstrVal);

    inArgsBound[0].lLbound = 0;
    inArgsBound[0].cElements = 3;
    psaInArgs = SafeArrayCreate(VT_VARIANT, 1, inArgsBound);
    if (psaInArgs == NULL) {
        hr = E_OUTOFMEMORY;
    }
    else {
        rgIndices[0] = 0;
        hr = SafeArrayPutElement(psaInArgs, rgIndices, &vt_RemoteHost);

        if (SUCCEEDED(hr)) {
            rgIndices[0] = 1;
            hr = SafeArrayPutElement(psaInArgs, rgIndices, &vt_ExternalPort);
        }

        if (SUCCEEDED(hr)) {
            rgIndices[0] = 2;
            hr = SafeArrayPutElement(psaInArgs, rgIndices, &vt_PortMappingProtocol);
        }

        if (SUCCEEDED(hr)) {
            varInArgs.vt = VT_VARIANT | VT_ARRAY;
            V_ARRAY(&varInArgs) = psaInArgs;
        }
    }

    // Invoke the function

    if (SUCCEEDED(hr)) {
        hr = GTOService->InvokeAction(bstrActionName, varInArgs, &varOutArgs, &varRet);

        if (varRet.vt == VT_BSTR) {
            returnStatus = std::to_string(varRet.bstrVal);
        }
    }

    if (SUCCEEDED(hr)) {
        psaOutArgs = V_ARRAY(&varOutArgs);

        rgIndices[0] = 0;
        hr = SafeArrayGetElement(psaOutArgs, rgIndices, (void*)&varTemp);
        if (SUCCEEDED(hr)) {
            *InternalPort = varTemp.uiVal;
        }

        if (SUCCEEDED(hr)) {
            rgIndices[0] = 1;
            hr = SafeArrayGetElement(psaOutArgs, rgIndices, (void*)&varTemp);
            if (SUCCEEDED(hr)) {
                *InternalClient = varTemp.bstrVal;
            }
        }

        if (SUCCEEDED(hr)) {
            rgIndices[0] = 2;
            hr = SafeArrayGetElement(psaOutArgs, rgIndices, (void*)&varTemp);
            if (SUCCEEDED(hr)) {
                *PortMappingEnabled = varTemp.boolVal;
            }
        }

        if (SUCCEEDED(hr)) {
            rgIndices[0] = 3;
            hr = SafeArrayGetElement(psaOutArgs, rgIndices, (void*)&varTemp);
            if (SUCCEEDED(hr)) {
                *PortMappingDescription = varTemp.bstrVal;
            }
        }

        if (SUCCEEDED(hr)) {
            rgIndices[0] = 4;
            hr = SafeArrayGetElement(psaOutArgs, rgIndices, (void*)&varTemp);
            if (SUCCEEDED(hr)) {
                *PortMappingLeaseDuration = varTemp.ulVal;
            }
        }
    }

    // Cleanup

    VariantClear(&vt_RemoteHost);
    VariantClear(&vt_ExternalPort);
    VariantClear(&vt_PortMappingProtocol);

    VariantClear(&varRet);
    VariantClear(&varTemp);

    if (NULL != psaInArgs) {
        SafeArrayDestroy(psaInArgs);
    }

    if (NULL != psaOutArgs) {
        SafeArrayDestroy(psaOutArgs);
    }

    SysFreeString(bstrActionName);

    return hr;
}


/// <summary>Invokes the AddPortMapping action from the UPnP device.</summary>
/// <param name="GTOService">UPnP service</param>
/// <param name="returnStatus">Return value from the service</param>
/// <param name="RemoteHost">Remote host</param>
/// <param name="ExternalPort">External port</param>
/// <param name="PortMappingProtocol">Port mapping protocol</param>
/// <param name="InternalPort">Internal port</param>
/// <param name="InternalClient">Internal client</param>
/// <param name="PortMappingEnabled">Port mapping enabled</param>
/// <param name="PortMappingDescription">Port mapping description</param>
/// <param name="PortMappingLeaseDuration">Port mapping lease duration in seconds</param>
/// <returns>Error code</returns>
HRESULT AddPortMapping(IUPnPService* GTOService, std::string returnStatus, BSTR RemoteHost, USHORT ExternalPort, BSTR PortMappingProtocol, USHORT InternalPort, BSTR InternalClient, VARIANT_BOOL PortMappingEnabled, BSTR PortMappingDescription, ULONG PortMappingLeaseDuration)
{
    HRESULT        hr             = S_OK;
    BSTR           bstrActionName = SysAllocString(L"AddPortMapping");
    SAFEARRAYBOUND inArgsBound[1];
    SAFEARRAY*     psaInArgs      = NULL;
    SAFEARRAY*     psaOutArgs     = NULL;
    LONG           rgIndices[1]   = { 0 };
    VARIANT        varInArgs      = { 0 };
    VARIANT        varOutArgs     = { 0 };
    VARIANT        varRet         = { 0 };
    VARIANT        varTemp        = { 0 };

    VariantInit(&varInArgs);
    VariantInit(&varOutArgs);
    VariantInit(&varRet);
    VariantInit(&varTemp);

    // Build the arguments to pass to the function

    TO_VARIANT(RemoteHost, VT_BSTR, bstrVal);
    TO_VARIANT(ExternalPort, VT_UI2, uiVal);
    TO_VARIANT(PortMappingProtocol, VT_BSTR, bstrVal);
    TO_VARIANT(InternalPort, VT_UI2, uiVal);
    TO_VARIANT(InternalClient, VT_BSTR, bstrVal);
    TO_VARIANT(PortMappingEnabled, VT_BOOL, boolVal);
    TO_VARIANT(PortMappingDescription, VT_BSTR, bstrVal);
    TO_VARIANT(PortMappingLeaseDuration, VT_UI4, ulVal);

    inArgsBound[0].lLbound = 0;
    inArgsBound[0].cElements = 8;
    psaInArgs = SafeArrayCreate(VT_VARIANT, 1, inArgsBound);
    if (psaInArgs == NULL) {
        hr = E_OUTOFMEMORY;
    }
    else {
        rgIndices[0] = 0;
        hr = SafeArrayPutElement(psaInArgs, rgIndices, &vt_RemoteHost);

        if (SUCCEEDED(hr)) {
            rgIndices[0] = 1;
            hr = SafeArrayPutElement(psaInArgs, rgIndices, &vt_ExternalPort);
        }

        if (SUCCEEDED(hr)) {
            rgIndices[0] = 2;
            hr = SafeArrayPutElement(psaInArgs, rgIndices, &vt_PortMappingProtocol);
        }

        if (SUCCEEDED(hr)) {
            rgIndices[0] = 3;
            hr = SafeArrayPutElement(psaInArgs, rgIndices, &vt_InternalPort);
        }

        if (SUCCEEDED(hr)) {
            rgIndices[0] = 4;
            hr = SafeArrayPutElement(psaInArgs, rgIndices, &vt_InternalClient);
        }

        if (SUCCEEDED(hr)) {
            rgIndices[0] = 5;
            hr = SafeArrayPutElement(psaInArgs, rgIndices, &vt_PortMappingEnabled);
        }

        if (SUCCEEDED(hr)) {
            rgIndices[0] = 6;
            hr = SafeArrayPutElement(psaInArgs, rgIndices, &vt_PortMappingDescription);
        }

        if (SUCCEEDED(hr)) {
            rgIndices[0] = 7;
            hr = SafeArrayPutElement(psaInArgs, rgIndices, &vt_PortMappingLeaseDuration);
        }

        if (SUCCEEDED(hr)) {
            varInArgs.vt = VT_VARIANT | VT_ARRAY;
            V_ARRAY(&varInArgs) = psaInArgs;
        }
    }

    // Invoke the function

    if (SUCCEEDED(hr)) {
        hr = GTOService->InvokeAction(bstrActionName, varInArgs, &varOutArgs, &varRet);

        if (varRet.vt == VT_BSTR) {
            returnStatus = std::to_string(varRet.bstrVal);
        }
    }

    if (SUCCEEDED(hr)) {
        psaOutArgs = V_ARRAY(&varOutArgs);
    }

    // Cleanup

    VariantClear(&vt_RemoteHost);
    VariantClear(&vt_ExternalPort);
    VariantClear(&vt_PortMappingProtocol);
    VariantClear(&vt_InternalPort);
    VariantClear(&vt_InternalClient);
    VariantClear(&vt_PortMappingEnabled);
    VariantClear(&vt_PortMappingDescription);
    VariantClear(&vt_PortMappingLeaseDuration);

    VariantClear(&varRet);
    VariantClear(&varTemp);

    if (NULL != psaInArgs) {
        SafeArrayDestroy(psaInArgs);
    }

    if (NULL != psaOutArgs) {
        SafeArrayDestroy(psaOutArgs);
    }

    SysFreeString(bstrActionName);

    return hr;
}


/// <summary>Invokes the DeletePortMapping action from the UPnP device.</summary>
/// <param name="GTOService">UPnP service</param>
/// <param name="returnStatus">Return value from the service</param>
/// <param name="RemoteHost">Remote host</param>
/// <param name="ExternalPort">External port</param>
/// <param name="PortMappingProtocol">Port mapping protocol</param>
/// <returns>Error code</returns>
HRESULT DeletePortMapping(IUPnPService* GTOService, std::string returnStatus, BSTR RemoteHost, USHORT ExternalPort, BSTR PortMappingProtocol)
{
    HRESULT        hr             = S_OK;
    BSTR           bstrActionName = SysAllocString(L"DeletePortMapping");
    SAFEARRAYBOUND inArgsBound[1];
    SAFEARRAY*     psaInArgs      = NULL;
    SAFEARRAY*     psaOutArgs     = NULL;
    LONG           rgIndices[1]   = { 0 };
    VARIANT        varInArgs      = { 0 };
    VARIANT        varOutArgs     = { 0 };
    VARIANT        varRet         = { 0 };
    VARIANT        varTemp        = { 0 };

    VariantInit(&varInArgs);
    VariantInit(&varOutArgs);
    VariantInit(&varRet);
    VariantInit(&varTemp);

    // Build the arguments to pass to the function

    TO_VARIANT(RemoteHost, VT_BSTR, bstrVal);
    TO_VARIANT(ExternalPort, VT_UI2, uiVal);
    TO_VARIANT(PortMappingProtocol, VT_BSTR, bstrVal);

    inArgsBound[0].lLbound = 0;
    inArgsBound[0].cElements = 3;
    psaInArgs = SafeArrayCreate(VT_VARIANT, 1, inArgsBound);
    if (psaInArgs == NULL) {
        hr = E_OUTOFMEMORY;
    }
    else {
        rgIndices[0] = 0;
        hr = SafeArrayPutElement(psaInArgs, rgIndices, &vt_RemoteHost);

        if (SUCCEEDED(hr)) {
            rgIndices[0] = 1;
            hr = SafeArrayPutElement(psaInArgs, rgIndices, &vt_ExternalPort);
        }

        if (SUCCEEDED(hr)) {
            rgIndices[0] = 2;
            hr = SafeArrayPutElement(psaInArgs, rgIndices, &vt_PortMappingProtocol);
        }

        if (SUCCEEDED(hr)) {
            varInArgs.vt = VT_VARIANT | VT_ARRAY;
            V_ARRAY(&varInArgs) = psaInArgs;
        }
    }

    // Invoke the function

    if (SUCCEEDED(hr)) {
        hr = GTOService->InvokeAction(bstrActionName, varInArgs, &varOutArgs, &varRet);
    }

    if (varRet.vt == VT_BSTR) {
        returnStatus = std::to_string(varRet.bstrVal);
    }

    if (SUCCEEDED(hr)) {
        psaOutArgs = V_ARRAY(&varOutArgs);
    }

    // Cleanup

    VariantClear(&vt_RemoteHost);
    VariantClear(&vt_ExternalPort);
    VariantClear(&vt_PortMappingProtocol);

    VariantClear(&varRet);
    VariantClear(&varTemp);

    if (NULL != psaInArgs) {
        SafeArrayDestroy(psaInArgs);
    }

    if (NULL != psaOutArgs) {
        SafeArrayDestroy(psaOutArgs);
    }

    SysFreeString(bstrActionName);

    return hr;
}


/// <summary>Invokes the GetExternalIPAddress action from the UPnP device.</summary>
/// <param name="GTOService">UPnP service</param>
/// <param name="returnStatus">Return value from the service</param>
/// <param name="ExternalIPAddress">Remote host</param>
/// <returns>Error code</returns>
HRESULT GetExternalIPAddress(IUPnPService* GTOService, std::string returnStatus, BSTR* ExternalIPAddress)
{
    HRESULT        hr             = S_OK;
    BSTR           bstrActionName = SysAllocString(L"GetExternalIPAddress");
    SAFEARRAYBOUND inArgsBound[1];
    SAFEARRAY*     psaInArgs      = NULL;
    SAFEARRAY*     psaOutArgs     = NULL;
    LONG           rgIndices[1]   = { 0 };
    VARIANT        varInArgs      = { 0 };
    VARIANT        varOutArgs     = { 0 };
    VARIANT        varRet         = { 0 };
    VARIANT        varTemp        = { 0 };

    VariantInit(&varInArgs);
    VariantInit(&varOutArgs);
    VariantInit(&varRet);
    VariantInit(&varTemp);

    // Build the arguments to pass to the function

    inArgsBound[0].lLbound = 0;
    inArgsBound[0].cElements = 0;
    psaInArgs = SafeArrayCreate(VT_VARIANT, 1, inArgsBound);
    if (psaInArgs == NULL) {
        hr = E_OUTOFMEMORY;
    }
    else {
        varInArgs.vt = VT_VARIANT | VT_ARRAY;
        V_ARRAY(&varInArgs) = psaInArgs;
    }

    // Invoke the function

    if (SUCCEEDED(hr)) {
        hr = GTOService->InvokeAction(bstrActionName, varInArgs, &varOutArgs, &varRet);
    }

    if (SUCCEEDED(hr)) {
        psaOutArgs = V_ARRAY(&varOutArgs);
        rgIndices[0] = 0;

        hr = SafeArrayGetElement(psaOutArgs, rgIndices, (void*)&varTemp);
        if (SUCCEEDED(hr)) {
            *ExternalIPAddress = varTemp.bstrVal;
        }
    }

    if (varRet.vt == VT_BSTR) {
        returnStatus = std::to_string(varRet.bstrVal);
    }

    // Cleanup

    VariantClear(&varRet);
    VariantClear(&varTemp);

    if (NULL != psaInArgs) {
        SafeArrayDestroy(psaInArgs);
    }

    if (NULL != psaOutArgs) {
        SafeArrayDestroy(psaOutArgs);
    }

    SysFreeString(bstrActionName);

    return hr;
}


/// <summary>Forwards the given port through the UPnP device.</summary>
/// <param name="externalPort">External port</param>
/// <param name="portLeaseDuration">Time in seconds for the port to be opened</param>
/// <param name="threaded">Whether the action should be executed on another thread</param>
void UPnPClient::forwardPort(unsigned short externalPort, unsigned long portLeaseDuration, bool threaded)
{
    if (threaded) {
        discoverThread->addJob([this, externalPort = externalPort, portLeaseDuration = portLeaseDuration]() { forwardPort(externalPort, portLeaseDuration, false); });
        return;
    }

    forwardPortStatus.clear();
    serviceAddPortMappingStatus = ServiceStatus::SERVICE_BUSY;
    BSTR         inRemoteHost               = SysAllocString(L"");
    USHORT       inExternalPort             = externalPort;
    BSTR         inPortMappingProtocol      = SysAllocString(L"UDP");
    USHORT       inInternalPort             = internalPort;
    BSTR         inInternalClient           = SysAllocString(std::to_wstring(internalIPAddress).c_str());
    VARIANT_BOOL inPortMappingEnabled       = VARIANT_TRUE;
    BSTR         inPortMappingDescription   = SysAllocString(L"RL port forward for local play");
    ULONG        inPortMappingLeaseDuration = portLeaseDuration;
    hResult = AddPortMapping(GTOServices[0], forwardPortStatus, inRemoteHost, inExternalPort, inPortMappingProtocol, inInternalPort, inInternalClient, inPortMappingEnabled, inPortMappingDescription, inPortMappingLeaseDuration);
    if (SUCCEEDED(hResult)) {
        USHORT       outInternalPort             = 0;
        BSTR         outInternalClient           = NULL;
        VARIANT_BOOL outPortMappingEnabled       = VARIANT_FALSE;
        BSTR         outPortMappingDescription   = NULL;
        ULONG        outPortMappingLeaseDuration = 0;
        hResult = GetSpecificPortMappingEntry(GTOServices[0], closePortStatus, inRemoteHost, inExternalPort, inPortMappingProtocol, &outInternalPort, &outInternalClient, &outPortMappingEnabled, &outPortMappingDescription, &outPortMappingLeaseDuration);
        if (SUCCEEDED(hResult)) {
            if (outInternalPort == inInternalPort && wcscmp(outInternalClient, inInternalClient) == 0 && outPortMappingEnabled == inPortMappingEnabled && outPortMappingLeaseDuration == inPortMappingLeaseDuration) {
                forwardPortStatus = std::to_string(externalPort) + " for " + internalIPAddress;
                serviceAddPortMappingStatus = ServiceStatus::SERVICE_ADDED_PORT_MAPPING;
                findOpenPorts(false);
            }
            else {
                forwardPortStatus += " " + std::string(externalIPAddress) + ":" + std::to_string(inExternalPort) + " forwarded to " + std::to_string(outInternalClient) + ":" + std::to_string(outInternalPort) + " (" + std::to_string(outPortMappingDescription) + ") for " + std::to_string(outPortMappingLeaseDuration) + " seconds";
                serviceAddPortMappingStatus = ServiceStatus::SERVICE_ERROR;
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
    
    serviceAddPortMappingStatus = ServiceStatus::SERVICE_ERROR;
}


/// <summary>Closes a forwarded port through the UPnP device.</summary>
/// <param name="externalPort">External port</param>
/// <param name="threaded">Whether the action should be executed on another thread</param>
void UPnPClient::closePort(unsigned short externalPort, bool threaded)
{
    if (threaded) {
        discoverThread->addJob([this, externalPort = externalPort]() { closePort(externalPort, false); });
        return;
    }

    closePortStatus.clear();
    serviceDeletePortMappingStatus = ServiceStatus::SERVICE_BUSY;
    BSTR   inRemoteHost          = SysAllocString(L"");
    USHORT inExternalPort        = externalPort;
    BSTR   inPortMappingProtocol = SysAllocString(L"UDP");
    hResult = DeletePortMapping(GTOServices[0], closePortStatus, inRemoteHost, inExternalPort, inPortMappingProtocol);
    if (SUCCEEDED(hResult)) {
        findOpenPorts(false);
        closePortStatus = std::to_string(externalPort) + " for " + internalIPAddress;
        serviceDeletePortMappingStatus = ServiceStatus::SERVICE_ADDED_PORT_MAPPING;
    }
    else {
        findOpenPorts(false);
        serviceDeletePortMappingStatus = ServiceStatus::SERVICE_ERROR;
    }
    SysFreeString(inRemoteHost);
    SysFreeString(inPortMappingProtocol);
}


/// <summary>Find open ports through the UPnP device.</summary>
/// <param name="threaded">Whether the action should be executed on another thread</param>
void UPnPClient::findOpenPorts(bool threaded)
{
    if (threaded) {
        discoverThread->addJob([this]() { findOpenPorts(false); });
        return;
    }

    openPorts.clear();
    USHORT inPortMappingNumberOfEntries = 0;
    hResult = S_OK;
    while (SUCCEEDED(hResult)) {
        serviceDeletePortMappingStatus = ServiceStatus::SERVICE_BUSY;
        BSTR         outRemoteHost               = NULL;
        USHORT       outExternalPort             = 0;
        BSTR         outPortMappingProtocol      = NULL;
        USHORT       outInternalPort             = 0;
        BSTR         outInternalClient           = NULL;
        VARIANT_BOOL outPortMappingEnabled       = VARIANT_FALSE;
        BSTR         outPortMappingDescription   = NULL;
        ULONG        outPortMappingLeaseDuration = 0;
        hResult = GetGenericPortMappingEntry(GTOServices[0], closePortStatus, inPortMappingNumberOfEntries, &outRemoteHost, &outExternalPort, &outPortMappingProtocol, &outInternalPort, &outInternalClient, &outPortMappingEnabled, &outPortMappingDescription, &outPortMappingLeaseDuration);
        if (SUCCEEDED(hResult)) {
            inPortMappingNumberOfEntries++;
            if (std::to_string(outInternalClient) == internalIPAddress) {
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
        serviceDeletePortMappingStatus = ServiceStatus::SERVICE_FINISHED;
    }
    else {
        serviceDeletePortMappingStatus = ServiceStatus::SERVICE_ERROR;
    }
}


/// <summary>Clears the saved UPnP devices.</summary>
void UPnPClient::clearDevices()
{
    if (GTOServices[0] != nullptr) {
        GTOServices[0]->Release();
        GTOServices[0] = nullptr;
    }
    if (GTOServices[1] != nullptr) {
        GTOServices[1]->Release();
        GTOServices[1] = nullptr;
    }
    if (GTODevice != nullptr) {
        GTODevice->Release();
        GTODevice = nullptr;
    }
}


/// <summary>Gets the internal IP address of the user.</summary>
/// <remarks>From https://docs.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-getipaddrtable</remarks>
/// <returns>Error code</returns>
HRESULT UPnPClient::getInternalIPAddress()
{
    int i;

    // Variables used by GetIpAddrTable
    PMIB_IPADDRTABLE pIPAddrTable;
    DWORD dwSize = 0;
    DWORD dwRetVal = 0;
    IN_ADDR IPAddr;

    // Before calling AddIPAddress we use GetIpAddrTable to get
    // an adapter to which we can add the IP.
    pIPAddrTable = (MIB_IPADDRTABLE*)malloc(sizeof(MIB_IPADDRTABLE));

    if (pIPAddrTable) {
        // Make an initial call to GetIpAddrTable to get the
        // necessary size into the dwSize variable
        if (GetIpAddrTable(pIPAddrTable, &dwSize, 0) == ERROR_INSUFFICIENT_BUFFER) {
            free(pIPAddrTable);
            pIPAddrTable = (MIB_IPADDRTABLE*)malloc(dwSize);
        }
    }
    if (pIPAddrTable) {
        // Make a second call to GetIpAddrTable to get the actual data we want
        dwRetVal = GetIpAddrTable(pIPAddrTable, &dwSize, 0);
        if (dwRetVal == NO_ERROR) {
            for (i = 0; i < (int)pIPAddrTable->dwNumEntries; i++) {
                if (pIPAddrTable->table[i].wType & MIB_IPADDR_PRIMARY) {
                    IPAddr.S_un.S_addr = (ULONG)pIPAddrTable->table[i].dwAddr;
                    if (is_rfc1918addr(inet_ntos(IPAddr))) {
                        internalIPAddress = std::string(inet_ntos(IPAddr));
                    }
                }
            }
        }
        free(pIPAddrTable);
        pIPAddrTable = NULL;
    }

    return HRESULT_FROM_WIN32(dwRetVal);
}


/// <summary>Find UPnP compatible device on the network.</summary>
/// <param name="threaded">Whether the action should be executed on another thread</param>
void UPnPClient::findDevices(bool threaded)
{
    if (threaded) {
        discoverThread->addJob([this]() { findDevices(false); });
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
    serviceAddPortMappingStatus = ServiceStatus::SERVICE_IDLE;
    serviceDeletePortMappingStatus = ServiceStatus::SERVICE_IDLE;
    discoveryStatus = DiscoveryStatus::DISCOVERY_BUSY;
    for (std::wstring device : deviceList) {
        DiscoverDevices(device);
        if (GTOServices[0] != nullptr) {
            break;
        }
    }

    if (SUCCEEDED(hResult) && GTOServices[0] != nullptr) {
        serviceAddPortMappingStatus = ServiceStatus::SERVICE_BUSY;
        discoveryStatus = DiscoveryStatus::DISCOVERY_FINISHED;
        BSTR bstrExternalIPAddress = NULL;
        hResult = GetExternalIPAddress(GTOServices[0], findDevicesStatus, &bstrExternalIPAddress);
        if (SUCCEEDED(hResult)) {
            strcpy_s(externalIPAddress, std::to_string(bstrExternalIPAddress).c_str());
            serviceAddPortMappingStatus = ServiceStatus::SERVICE_GOT_EXT_IP;
        }
        else {
            // Swap WANPPPConnection and WANIPConnection, so GTOServices[0] always works.
            IUPnPService* tmp = GTOServices[0];
            GTOServices[0] = GTOServices[1];
            GTOServices[1] = tmp;
            hResult = GetExternalIPAddress(GTOServices[0], findDevicesStatus, &bstrExternalIPAddress);
            if (SUCCEEDED(hResult)) {
                // InternalClient
                strcpy_s(externalIPAddress, std::to_string(bstrExternalIPAddress).c_str());
                serviceAddPortMappingStatus = ServiceStatus::SERVICE_GOT_EXT_IP;
            }
            else {
                serviceAddPortMappingStatus = ServiceStatus::SERVICE_ERROR;
            }
        }

        if (SUCCEEDED(hResult)) {
            hResult = getInternalIPAddress();
            if (!SUCCEEDED(hResult)) {
                serviceAddPortMappingStatus = ServiceStatus::SERVICE_ERROR;
            }
            else if (internalIPAddress.empty()) {
                forwardPortStatus = "";
                serviceAddPortMappingStatus = ServiceStatus::SERVICE_ERROR;
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
        BSTR bstrFriendlyName = NULL;
        HRESULT hr = pDevice->get_FriendlyName(&bstrFriendlyName);
        if (SUCCEEDED(hr)) {
            deviceFriendlyName = std::to_string(bstrFriendlyName);
        }
        else {
            deviceFriendlyName = "Unknown Name\"\n\"" + printHResult(hr) + "\"";
        }
    }

    return deviceFriendlyName;
}


/// <summary>Gets the current discovery status.</summary>
/// <returns>A description of the current discovery status</returns>
std::string UPnPClient::getDiscoveryStatus()
{
    switch (discoveryStatus)
    {
    case DiscoveryStatus::DISCOVERY_IDLE:
        return "";
    case DiscoveryStatus::DISCOVERY_ERROR:
        if (SUCCEEDED(hResult)) {
            return "Could not find any compatible devices. " + findDevicesStatus;
        }
        else {
            return "Error: " + printHResult(hResult) + " " + findDevicesStatus;
        }
    case DiscoveryStatus::DISCOVERY_BUSY:
    case DiscoveryStatus::DISCOVERY_FOUND_GTO:
        return "Searching for UPnP compatible devices.";
    case DiscoveryStatus::DISCOVERY_FINISHED:
        return "Found a UPnP compatible device. \"" + getDeviceFriendlyName(GTODevice) + "\"";
    default:
        break;
    }

    return "Unknown status";
}


/// <summary>Gets the current port forwarding status.</summary>
/// <returns>A description of the current port forwarding status</returns>
std::string UPnPClient::getforwardPortStatus()
{
    switch (serviceAddPortMappingStatus)
    {
    case ServiceStatus::SERVICE_IDLE:
    case ServiceStatus::SERVICE_FINISHED:
        return "Waiting for input.";
    case ServiceStatus::SERVICE_ERROR:
        return "Error: " + printHResult(hResult) + " " + forwardPortStatus;
    case ServiceStatus::SERVICE_BUSY:
        return "Connecting with service.";
    case ServiceStatus::SERVICE_GOT_EXT_IP:
        return "Succesfully connected with service as " + internalIPAddress + ".";
    case ServiceStatus::SERVICE_ADDED_PORT_MAPPING:
        return "Succesfully redirected port " + forwardPortStatus;
    default:
        break;
    }

    return "Unknown status " + forwardPortStatus;
}


/// <summary>Gets the current forwarded port closing status.</summary>
/// <returns>A description of the current forwarded port closing status</returns>
std::string UPnPClient::getclosePortStatus()
{
    switch (serviceDeletePortMappingStatus)
    {
    case ServiceStatus::SERVICE_IDLE:
    case ServiceStatus::SERVICE_FINISHED:
        return "";
    case ServiceStatus::SERVICE_ERROR:
        return "Error: " + printHResult(hResult) + " " + closePortStatus;
    case ServiceStatus::SERVICE_BUSY:
        return "Connecting with service.";
    case ServiceStatus::SERVICE_ADDED_PORT_MAPPING:
        return "Succesfully closed port " + closePortStatus;
    default:
        break;
    }

    return "Unknown status " + closePortStatus;
}


/// <summary>Initializes the discovery thread <see cref="WorkerThread"/>.</summary>
UPnPClient::UPnPClient()
{
    discoverThread = new WorkerThread();
}


/// <summary>Clears the saved devices and exits the discovery thread <see cref="WorkerThread"/>.</summary>
UPnPClient::~UPnPClient()
{
    clearDevices();
    delete discoverThread;
}