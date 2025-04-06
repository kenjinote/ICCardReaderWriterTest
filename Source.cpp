#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#pragma comment(lib, "winscard")

#include <windows.h>
#include <winscard.h>

TCHAR szClassName[] = TEXT("Window");

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static HWND hButton;
    static HWND hEdit;
    switch (msg)
    {
    case WM_CREATE:
        hButton = CreateWindow(TEXT("BUTTON"), TEXT("ICカードリーダライタ情報取得"), WS_VISIBLE | WS_CHILD, 0, 0, 0, 0, hWnd, (HMENU)IDOK, ((LPCREATESTRUCT)lParam)->hInstance, 0);
        hEdit = CreateWindowEx(WS_EX_CLIENTEDGE, TEXT("EDIT"), 0, WS_VISIBLE | WS_CHILD | ES_MULTILINE | ES_AUTOHSCROLL | ES_AUTOVSCROLL | ES_READONLY, 0, 0, 0, 0, hWnd, 0, ((LPCREATESTRUCT)lParam)->hInstance, 0);
        SendMessage(hWnd, WM_DPICHANGED, 0, 0);
        break;
    case WM_SIZE:
        MoveWindow(hButton, 10, 10, 256, 32, TRUE);
        MoveWindow(hEdit, 10, 50, LOWORD(lParam) - 20, HIWORD(lParam) - 60, TRUE);
        break;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK)
        {
            SCARDCONTEXT hContext;
            LONG lResult = SCardEstablishContext(SCARD_SCOPE_USER, NULL, NULL, &hContext);
            if (lResult != SCARD_S_SUCCESS) {
                MessageBox(hWnd, TEXT("Failed to establish context"), TEXT("Error"), MB_OK | MB_ICONERROR);
                break;
            }

            // カードリーダーのリストを取得
            LPTSTR mszReaders;
            DWORD dwReaders;
            lResult = SCardListReaders(hContext, NULL, NULL, &dwReaders);
            if (lResult != SCARD_S_SUCCESS) {
                MessageBox(hWnd, TEXT("Failed to list readers"), TEXT("Error"), MB_OK | MB_ICONERROR);
                SCardReleaseContext(hContext);
                break;
            }

            mszReaders = (LPWSTR)malloc(dwReaders * sizeof(WCHAR));
            lResult = SCardListReaders(hContext, NULL, mszReaders, &dwReaders);
            if (lResult != SCARD_S_SUCCESS) {
                MessageBox(hWnd, TEXT("Failed to list readers"), TEXT("Error"), MB_OK | MB_ICONERROR);
                free(mszReaders);
                SCardReleaseContext(hContext);
                break;
            }

            // 最初のリーダーを使用
            LPWSTR szReader = mszReaders;
            SCARDHANDLE hCard;
            DWORD dwActiveProtocol;
            lResult = SCardConnect(hContext, szReader, SCARD_SHARE_SHARED, SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1, &hCard, &dwActiveProtocol);
            if (lResult != SCARD_S_SUCCESS) {
                MessageBox(hWnd, TEXT("Failed to connect to card"), TEXT("Error"), MB_OK | MB_ICONERROR);
                free(mszReaders);
                SCardReleaseContext(hContext);
                break;
            }

            // カード情報を取得
            TCHAR szCardInfo[256];
            DWORD dwCardInfoLen = _countof(szCardInfo);
            DWORD dwState = 0;
            DWORD dwProtocol = 0;
            BYTE bAtr[256];
            DWORD dwbAtr = _countof(bAtr);

            lResult = SCardStatus(hCard, szCardInfo, &dwCardInfoLen, &dwState, &dwProtocol, bAtr, &dwbAtr);
            if (lResult != SCARD_S_SUCCESS) {
                MessageBox(hWnd, TEXT("Failed to get card status"), TEXT("Error"), MB_OK | MB_ICONERROR);
            } else {
                WCHAR szText[1024] = {};
				lstrcpy(szText, L"Card Info: ");
				lstrcat(szText, szCardInfo);
				lstrcat(szText, L"\r\nState: ");
				switch (dwState) {
				case SCARD_ABSENT:
					lstrcat(szText, L"Absent");
					break;
				case SCARD_PRESENT:
					lstrcat(szText, L"Present");
					break;
				case SCARD_SWALLOWED:
					lstrcat(szText, L"Swallowed");
					break;
                case SCARD_POWERED:
                    lstrcat(szText, L"Powered");
                    break;
                case SCARD_NEGOTIABLE:
                    lstrcat(szText, L"Negotiable");
                    break;
                case SCARD_SPECIFIC:
                    lstrcat(szText, L"Specific");
                    break;
                default:
					lstrcat(szText, L"Unknown");
					break;
				}
				lstrcat(szText, L"\r\nProtocol: ");
				switch (dwProtocol) {
				case SCARD_PROTOCOL_T0:
					lstrcat(szText, L"T0");
					break;
				case SCARD_PROTOCOL_T1:
					lstrcat(szText, L"T1");
					break;
				default:
					lstrcat(szText, L"Unknown");
					break;
				}
				lstrcat(szText, L"\r\nATR: ");                
                for (int i = 0; i < dwbAtr; i++) {
                    WCHAR szTemp[3];
                    wsprintf(szTemp, L"%02X", bAtr[i]);
                    lstrcat(szText, szTemp);
                }
                lstrcat(szText, L"\r\n");
                SetWindowText(hEdit, szText);
            }

            // リソースを解放
            SCardDisconnect(hCard, SCARD_LEAVE_CARD);
            free(mszReaders);
            SCardReleaseContext(hContext);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPreInst, LPWSTR pCmdLine, int nCmdShow)
{
	MSG msg;
	WNDCLASS wndclass = {
		CS_HREDRAW | CS_VREDRAW,
		WndProc,
		0,
		0,
		hInstance,
		0,
		LoadCursor(0,IDC_ARROW),
		(HBRUSH)(COLOR_WINDOW + 1),
		0,
		szClassName
	};
	RegisterClass(&wndclass);
	HWND hWnd = CreateWindow(
		szClassName,
		TEXT("ICカードリーダライタ"),
		WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
		CW_USEDEFAULT,
		0,
		CW_USEDEFAULT,
		0,
		0,
		0,
		hInstance,
		0
	);
	ShowWindow(hWnd, SW_SHOWDEFAULT);
	UpdateWindow(hWnd);
	while (GetMessage(&msg, 0, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return (int)msg.wParam;
}
