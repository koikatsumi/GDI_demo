#include <windows.h>
#include <iostream>
#include <ctime>
#include <cmath>
#include <cwchar>
#include <vector>
#include <gdiplus.h>

#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdiplus.lib")

const double PI = 3.14159265358979323846;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == WM_DESTROY) {
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

// --- 3D MAP DEFINITION (0 = empty space, 1 = Betelgeuse wall) ---
const int MAP_WIDTH = 16;
const int MAP_HEIGHT = 16;
const char map[] = "################"
                   "#..............#"
                   "#..####....###.#"
                   "#..#.........#.#"
                   "#..#..####...#.#"
                   "#..#.....#...#.#"
                   "#..####..#...#.#"
                   "#........#.....#"
                   "#..#######.....#"
                   "#..#...........#"
                   "#..#..##########"
                   "#..#...........#"
                   "#..##########..#"
                   "#..............#"
                   "#..............#"
                   "################";

bool AllAliensDefeated(const bool (&aliensAlive)[4][10]) {
    for (int row = 0; row < 4; row++) {
        for (int column = 0; column < 10; column++) {
            if (aliensAlive[row][column]) return false;
        }
    }
    return true;
}

void DrawQRCode(HDC hdc, Gdiplus::Image* qrCode, int width, int height) {
    if (!qrCode || qrCode->GetLastStatus() != Gdiplus::Ok) return;
    
    Gdiplus::Graphics graphics(hdc);
    graphics.SetInterpolationMode(Gdiplus::InterpolationModeNearestNeighbor);
    
    int qrSize = height < 360 ? height - 80 : 300;
    int qrX = (width - qrSize) / 2;
    int qrY = (height - qrSize) / 2;
    graphics.DrawImage(qrCode, Gdiplus::Rect(qrX, qrY, qrSize, qrSize));
    
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(255, 255, 255));
    const wchar_t* message = L"THANKS FOR PLAYING - SCAN TO VISIT KOI";
    TextOutW(hdc, 18, 18, message, static_cast<int>(wcslen(message)));
}

void RenderSpaceInvaders(HDC hdc, int width, int height, double time, double& playerShipX, bool (&aliensAlive)[4][10], bool& playerShotActive, int& playerShotX, int& playerShotY, double& formationX, double& formationY, double& formationDirection, bool& enemyShotActive, int& enemyShotX, int& enemyShotY, double& enemyShotCooldown) {
    const int renderWidth = 320;
    const int renderHeight = 213;
    std::vector<RGBQUAD> pixels(renderWidth * renderHeight);
    
    auto putPixel = [&pixels](int x, int y, int red, int green, int blue) {
        if (x < 0 || x >= 320 || y < 0 || y >= 213) return;
        RGBQUAD& pixel = pixels[y * 320 + x];
        pixel.rgbRed = static_cast<BYTE>(red);
        pixel.rgbGreen = static_cast<BYTE>(green);
        pixel.rgbBlue = static_cast<BYTE>(blue);
        pixel.rgbReserved = 0;
    };
    
    for (int py = 0; py < renderHeight; py++) {
        for (int px = 0; px < renderWidth; px++) {
            int stars = (px * 37 + py * 91 + static_cast<int>(time * 20.0)) & 31;
            putPixel(px, py, 1, 2 + stars / 12, 12 + stars);
        }
    }
    
    const char* alien = "01110111111010101111101010";
    
    if (GetAsyncKeyState(VK_LEFT) & 0x8000 || GetAsyncKeyState('A') & 0x8000) {
        playerShipX -= 3.0;
    }
    if (GetAsyncKeyState(VK_RIGHT) & 0x8000 || GetAsyncKeyState('D') & 0x8000) {
        playerShipX += 3.0;
    }
    if (playerShipX < 20.0) playerShipX = 20.0;
    if (playerShipX > 300.0) playerShipX = 300.0;
    
    double playerScreenX = playerShipX;
    formationX += formationDirection * 0.30;
    
    if (formationX <= 8.0 || formationX >= 25.0) {
        formationDirection = -formationDirection;
        formationY += 4.0;
    }
    
    if (!playerShotActive) {
        playerShotActive = true;
        playerShotX = static_cast<int>(playerScreenX);
        playerShotY = 178;
    } else {
        playerShotY -= 3;
        if (playerShotY < 10) playerShotActive = false;
    }
    
    if (playerShotActive) {
        for (int row = 0; row < 4; row++) {
            for (int column = 0; column < 10; column++) {
                int alienX = static_cast<int>(formationX) + column * 28;
                int alienY = static_cast<int>(formationY) + row * 24;
                if (aliensAlive[row][column] && playerShotX >= alienX && playerShotX < alienX + 15 && playerShotY >= alienY && playerShotY < alienY + 15) {
                    aliensAlive[row][column] = false;
                    playerShotActive = false;
                }
            }
        }
    }
    
    for (int row = 0; row < 4; row++) {
        for (int column = 0; column < 10; column++) {
            if (!aliensAlive[row][column]) continue;
            
            int baseX = static_cast<int>(formationX) + column * 28;
            int baseY = static_cast<int>(formationY) + row * 24;
            int red = row == 0 ? 255 : 80;
            int green = row == 0 ? 20 : 180;
            int blue = row == 0 ? 210 : 255;
            
            for (int spriteY = 0; spriteY < 5; spriteY++) {
                for (int spriteX = 0; spriteX < 5; spriteX++) {
                    if (alien[spriteY * 5 + spriteX] == '1') {
                        for (int blockY = 0; blockY < 3; blockY++) {
                            for (int blockX = 0; blockX < 3; blockX++) {
                                putPixel(baseX + spriteX * 3 + blockX, baseY + spriteY * 3 + blockY, red, green, blue);
                            }
                        }
                    }
                }
            }
        }
    }
    
    int playerX = static_cast<int>(playerScreenX);
    for (int y = 0; y < 5; y++) {
        for (int x = -18; x <= 18; x++) {
            int width = y == 0 ? 2 : y == 1 ? 8 : y == 2 ? 14 : 18;
            if (std::abs(x) <= width) putPixel(playerX + x, 190 + y, 40, 255, 120);
        }
    }
    
    if (playerShotActive) {
        for (int y = 0; y < 9; y++) putPixel(playerShotX, playerShotY + y, 255, 255, 80);
    }
    
    if (enemyShotActive) {
        enemyShotY += 2;
        if (enemyShotY > renderHeight) enemyShotActive = false;
    } else {
        enemyShotCooldown -= 1.0;
        if (enemyShotCooldown <= 0.0) {
            int targetColumn = static_cast<int>((playerScreenX - formationX) / 28.0 + 0.5);
            if (targetColumn < 0) targetColumn = 0;
            if (targetColumn > 9) targetColumn = 9;
            
            int firingRow = -1;
            for (int row = 3; row >= 0; row--) {
                if (aliensAlive[row][targetColumn]) {
                    firingRow = row;
                    break;
                }
            }
            
            if (firingRow >= 0) {
                enemyShotActive = true;
                enemyShotX = static_cast<int>(formationX) + targetColumn * 28 + 6;
                enemyShotY = static_cast<int>(formationY) + firingRow * 24 + 18;
            }
            enemyShotCooldown = 150.0;
        }
    }
    
    if (enemyShotActive) {
        for (int y = 0; y < 8; y++) putPixel(enemyShotX, enemyShotY + y, 255, 60, 40);
    }
    
    BITMAPINFO bitmapInfo = {};
    bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bitmapInfo.bmiHeader.biWidth = renderWidth;
    bitmapInfo.bmiHeader.biHeight = -renderHeight;
    bitmapInfo.bmiHeader.biPlanes = 1;
    bitmapInfo.bmiHeader.biBitCount = 32;
    bitmapInfo.bmiHeader.biCompression = BI_RGB;
    StretchDIBits(hdc, 0, 0, width, height, 0, 0, renderWidth, renderHeight, pixels.data(), &bitmapInfo, DIB_RGB_COLORS, SRCCOPY);
}

void DrawScrollingCredit(HDC hdc, HFONT font, int width, int height, int& position) {
    const wchar_t* credit = L"made by Koi";
    HFONT oldFont = static_cast<HFONT>(SelectObject(hdc, font));
    SIZE textSize = {};
    GetTextExtentPoint32W(hdc, credit, static_cast<int>(wcslen(credit)), &textSize);
    
    int textY = height - textSize.cy - 12;
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(0, 0, 0));
    TextOutW(hdc, position + 2, textY + 2, credit, static_cast<int>(wcslen(credit)));
    SetTextColor(hdc, RGB(255, 0, 220));
    TextOutW(hdc, position, textY, credit, static_cast<int>(wcslen(credit)));
    
    position -= 4;
    if (position < -textSize.cx) {
        position = width;
    }
    SelectObject(hdc, oldFont);
}

void DrawDodecahedron(HDC hdc, int screenWidth, int screenHeight, double rotation, double centerX, double centerY) {
    const double phi = 1.61803398875;
    const double invPhi = 1.0 / phi;
    const double vertices[20][3] = {
        {-1, -1, -1}, {-1, -1, 1}, {-1, 1, -1}, {-1, 1, 1},
        {1, -1, -1}, {1, -1, 1}, {1, 1, -1}, {1, 1, 1},
        {0, -invPhi, -phi}, {0, -invPhi, phi}, {0, invPhi, -phi}, {0, invPhi, phi},
        {-invPhi, -phi, 0}, {-invPhi, phi, 0}, {invPhi, -phi, 0}, {invPhi, phi, 0},
        {-phi, 0, -invPhi}, {phi, 0, -invPhi}, {-phi, 0, invPhi}, {phi, 0, invPhi}
    };
    
    POINT projected[20];
    double cosine = std::cos(rotation);
    double sine = std::sin(rotation);
    int projectedCenterX = static_cast<int>(centerX);
    int projectedCenterY = static_cast<int>(centerY);
    
    for (int i = 0; i < 20; i++) {
        double x = vertices[i][0];
        double y = vertices[i][1];
        double z = vertices[i][2];
        double rotatedX = x * cosine - z * sine;
        double rotatedZ = x * sine + z * cosine;
        double rotatedY = y * std::cos(rotation * 0.73) - rotatedZ * std::sin(rotation * 0.73);
        rotatedZ = y * std::sin(rotation * 0.73) + rotatedZ * std::cos(rotation * 0.73);
        double perspective = 1.0 / (rotatedZ * 0.12 + 1.8);
        projected[i].x = projectedCenterX + static_cast<LONG>(rotatedX * 210.0 * perspective);
        projected[i].y = projectedCenterY + static_cast<LONG>(rotatedY * 210.0 * perspective);
    }
    
    HPEN pen = CreatePen(PS_SOLID, 3, RGB(255, 0, 220));
    HPEN oldPen = static_cast<HPEN>(SelectObject(hdc, pen));
    
    for (int first = 0; first < 20; first++) {
        for (int second = first + 1; second < 20; second++) {
            double dx = vertices[first][0] - vertices[second][0];
            double dy = vertices[first][1] - vertices[second][1];
            double dz = vertices[first][2] - vertices[second][2];
            double distanceSquared = dx * dx + dy * dy + dz * dz;
            if (distanceSquared > 1.45 && distanceSquared < 1.70) {
                MoveToEx(hdc, projected[first].x, projected[first].y, NULL);
                LineTo(hdc, projected[second].x, projected[second].y);
            }
        }
    }
    
    SelectObject(hdc, oldPen);
    DeleteObject(pen);
}

void RunDesktopIntro() {
    const int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    const int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    HDC hdcDesktop = GetDC(NULL);
    if (!hdcDesktop) {
        return;
    }
    
    HDC hdcSnapshot = CreateCompatibleDC(hdcDesktop);
    HBITMAP hbmSnapshot = CreateCompatibleBitmap(hdcDesktop, screenWidth, screenHeight);
    if (!hdcSnapshot || !hbmSnapshot) {
        if (hbmSnapshot) DeleteObject(hbmSnapshot);
        if (hdcSnapshot) DeleteDC(hdcSnapshot);
        ReleaseDC(NULL, hdcDesktop);
        return;
    }
    
    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdcSnapshot, hbmSnapshot);
    BitBlt(hdcSnapshot, 0, 0, screenWidth, screenHeight, hdcDesktop, 0, 0, SRCCOPY);
    
    const int meltStripWidth = 6;
    const int meltStripCount = (screenWidth + meltStripWidth - 1) / meltStripWidth;
    std::vector<double> meltOffsets(meltStripCount, 0.0);
    std::vector<double> meltSpeeds(meltStripCount, 0.0);
    std::vector<double> meltDelays(meltStripCount, 0.0);
    
    for (int strip = 0; strip < meltStripCount; strip++) {
        meltSpeeds[strip] = 35.0 + rand() % 36;
        meltDelays[strip] = static_cast<double>(rand() % 2500) / 1000.0;
    }
    
    ULONGLONG meltStartTime = GetTickCount64();
    ULONGLONG previousFrameTime = meltStartTime;
    ULONGLONG meltEndTime = meltStartTime + 45000;
    double dodecahedronX = screenWidth / 2.0;
    double dodecahedronY = screenHeight / 2.0;
    double dodecahedronVelocityX = 180.0;
    double dodecahedronVelocityY = 125.0;
    
    while (GetTickCount64() < meltEndTime && !(GetAsyncKeyState(VK_ESCAPE) & 0x8000)) {
        BitBlt(hdcDesktop, 0, 0, screenWidth, screenHeight, hdcSnapshot, 0, 0, SRCCOPY);
        ULONGLONG currentTime = GetTickCount64();
        double deltaSeconds = static_cast<double>(currentTime - previousFrameTime) / 1000.0;
        previousFrameTime = currentTime;
        if (deltaSeconds > 0.2) deltaSeconds = 0.2;
        double elapsed = static_cast<double>(currentTime - meltStartTime) / 1000.0;
        
        for (int strip = 0; strip < meltStripCount; strip++) {
            double stripTime = elapsed - meltDelays[strip];
            if (stripTime <= 0.0) {
                continue;
            }
            if (meltOffsets[strip] < screenHeight - 1) {
                meltOffsets[strip] += meltSpeeds[strip] * deltaSeconds;
                if (meltOffsets[strip] > screenHeight - 1) {
                    meltOffsets[strip] = screenHeight - 1;
                }
            }
            int x = strip * meltStripWidth;
            int width = meltStripWidth;
            if (x + width > screenWidth) width = screenWidth - x;
            int drop = static_cast<int>(meltOffsets[strip]);
            PatBlt(hdcDesktop, x, 0, width, drop, BLACKNESS);
            BitBlt(hdcDesktop, x, drop, width, screenHeight - drop, hdcSnapshot, x, 0, SRCCOPY);
        }
        
        dodecahedronX += dodecahedronVelocityX * deltaSeconds;
        dodecahedronY += dodecahedronVelocityY * deltaSeconds;
        if (dodecahedronX < 260.0 || dodecahedronX > screenWidth - 260.0) {
            dodecahedronVelocityX = -dodecahedronVelocityX;
            dodecahedronX = dodecahedronX < 260.0 ? 260.0 : screenWidth - 260.0;
        }
        if (dodecahedronY < 260.0 || dodecahedronY > screenHeight - 260.0) {
            dodecahedronVelocityY = -dodecahedronVelocityY;
            dodecahedronY = dodecahedronY < 260.0 ? 260.0 : screenHeight - 260.0;
        }
        DrawDodecahedron(hdcDesktop, screenWidth, screenHeight, static_cast<double>(currentTime) * 0.002, dodecahedronX, dodecahedronY);
        int scanLine = static_cast<int>(elapsed * 12.0) % screenHeight;
        PatBlt(hdcDesktop, 0, scanLine, screenWidth, 1, DSTINVERT);
        Sleep(16);
    }
    
    ULONGLONG glitchEndTime = GetTickCount64() + 15000;
    ULONGLONG previousGlitchFrameTime = GetTickCount64();
    while (GetTickCount64() < glitchEndTime && !(GetAsyncKeyState(VK_ESCAPE) & 0x8000)) {
        BitBlt(hdcDesktop, 0, 0, screenWidth, screenHeight, hdcSnapshot, 0, 0, SRCCOPY);
        ULONGLONG currentGlitchFrameTime = GetTickCount64();
        double glitchDeltaSeconds = static_cast<double>(currentGlitchFrameTime - previousGlitchFrameTime) / 1000.0;
        previousGlitchFrameTime = currentGlitchFrameTime;
        if (glitchDeltaSeconds > 0.2) glitchDeltaSeconds = 0.2;
        
        for (int strip = 0; strip < 5; strip++) {
            int y = rand() % screenHeight;
            int height = 8 + rand() % 40;
            int offset = (rand() % 80) - 40;
            BitBlt(hdcDesktop, 0, y, screenWidth, height, hdcSnapshot, offset, y, SRCCOPY);
        }
        
        dodecahedronX += dodecahedronVelocityX * glitchDeltaSeconds;
        dodecahedronY += dodecahedronVelocityY * glitchDeltaSeconds;
        if (dodecahedronX < 260.0 || dodecahedronX > screenWidth - 260.0) {
            dodecahedronVelocityX = -dodecahedronVelocityX;
            dodecahedronX = dodecahedronX < 260.0 ? 260.0 : screenWidth - 260.0;
        }
        if (dodecahedronY < 260.0 || dodecahedronY > screenHeight - 260.0) {
            dodecahedronVelocityY = -dodecahedronVelocityY;
            dodecahedronY = dodecahedronY < 260.0 ? 260.0 : screenHeight - 260.0;
        }
        DrawDodecahedron(hdcDesktop, screenWidth, screenHeight, static_cast<double>(currentGlitchFrameTime) * 0.002, dodecahedronX, dodecahedronY);
        Sleep(16);
    }
    
    BitBlt(hdcDesktop, 0, 0, screenWidth, screenHeight, hdcSnapshot, 0, 0, SRCCOPY);
    SelectObject(hdcSnapshot, hOldBitmap);
    DeleteObject(hbmSnapshot);
    DeleteDC(hdcSnapshot);
    ReleaseDC(NULL, hdcDesktop);
}

int main() {
    HWND existingHwnd = FindWindowA(NULL, "the end");
    if (existingHwnd) {
        PostMessage(existingHwnd, WM_CLOSE, 0, 0);
        Sleep(50);
    }
    
    SetProcessDPIAware();
    srand(static_cast<unsigned int>(time(NULL)));
    RunDesktopIntro();
    
    HINSTANCE instance = GetModuleHandleW(NULL);
    WNDCLASSW windowClass = {};
    windowClass.hInstance = instance;
    windowClass.lpfnWndProc = WindowProc;
    windowClass.lpszClassName = L"Solaris3DWindowClass";
    windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    windowClass.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
    RegisterClassW(&windowClass);
    
    const int windowWidth = 960;
    const int windowHeight = 640;
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    RECT windowRect = { 0, 0, windowWidth, windowHeight };
    AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);
    int outerWidth = windowRect.right - windowRect.left;
    int outerHeight = windowRect.bottom - windowRect.top;
    
    HWND hwnd = CreateWindowExW(
        0, windowClass.lpszClassName, L"Solaris 3D Realtime",
        WS_OVERLAPPEDWINDOW,
        (screenWidth - outerWidth) / 2, (screenHeight - outerHeight) / 2,
        outerWidth, outerHeight,
        NULL, NULL, instance, NULL);
    
    if (!hwnd) {
        return 1;
    }
    
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    
    RECT clientRect;
    GetClientRect(hwnd, &clientRect);
    screenWidth = clientRect.right - clientRect.left;
    screenHeight = clientRect.bottom - clientRect.top;
    
    HDC hdcWindow = GetDC(hwnd);
    HDC hdcMem = CreateCompatibleDC(hdcWindow);
    HBITMAP hbmMem = CreateCompatibleBitmap(hdcWindow, screenWidth, screenHeight);
    HBITMAP hOldBm = (HBITMAP)SelectObject(hdcMem, hbmMem);
    
    HFONT creditFont = CreateFontW(30, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, NONANTIALIASED_QUALITY, FIXED_PITCH | FF_MODERN, L"Terminal");
    int creditPosition = screenWidth;
    
    double playerX = 2.0;
    double playerY = 2.0;
    double playerAngle = 0.0;
    double fov = PI / 3.0;
    double maxDepth = 16.0;
    double moveSpeed = 0.08;
    double rotSpeed = 0.04;
    const double objectiveX = 2.0;
    const double objectiveY = 13.0;
    bool objectiveReached = false;
    double fractalZoom = 1.0;
    double fractalRotation = 0.0;
    bool spaceInvadersFinished = false;
    
    double playerShipX = 160.0;
    bool aliensAlive[4][10] = {};
    for (int row = 0; row < 4; row++) {
        for (int column = 0; column < 10; column++) {
            aliensAlive[row][column] = true;
        }
    }
    bool playerShotActive = false;
    int playerShotX = 0;
    int playerShotY = 0;
    double formationX = 8.0;
    double formationY = 28.0;
    double formationDirection = 1.0;
    bool enemyShotActive = false;
    int enemyShotX = 0;
    int enemyShotY = 0;
    double enemyShotCooldown = 45.0;
    
    time_t startTime = time(NULL);
    time_t duration = 30;
    
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken = 0;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
    
    wchar_t executablePath[MAX_PATH] = {};
    GetModuleFileNameW(NULL, executablePath, MAX_PATH);
    wchar_t* lastSlash = wcsrchr(executablePath, L'\\');
    if (lastSlash) *(lastSlash + 1) = L'\0';
    wcscat_s(executablePath, MAX_PATH, L"qr_code.png");
    Gdiplus::Image qrCode(executablePath);
    
    bool running = true;
    while (running && (objectiveReached || time(NULL) - startTime < duration) && !(GetAsyncKeyState(VK_ESCAPE) & 0x8000)) {
        MSG message;
        while (PeekMessage(&message, NULL, 0, 0, PM_REMOVE)) {
            if (message.message == WM_QUIT) {
                running = false;
            } else {
                TranslateMessage(&message);
                DispatchMessage(&message);
            }
        }
        
        if (!running) {
            break;
        }
        
        if (objectiveReached) {
            fractalZoom *= 1.002;
            fractalRotation += 0.008;
            if (fractalZoom > 1.25) {
                fractalZoom = 1.0;
            }
            if (spaceInvadersFinished) {
                PatBlt(hdcMem, 0, 0, screenWidth, screenHeight, BLACKNESS);
                DrawQRCode(hdcMem, &qrCode, screenWidth, screenHeight);
            } else {
                RenderSpaceInvaders(hdcMem, screenWidth, screenHeight, fractalRotation, playerShipX, aliensAlive, playerShotActive, playerShotX, playerShotY, formationX, formationY, formationDirection, enemyShotActive, enemyShotX, enemyShotY, enemyShotCooldown);
                spaceInvadersFinished = AllAliensDefeated(aliensAlive);
            }
            DrawScrollingCredit(hdcMem, creditFont, screenWidth, screenHeight, creditPosition);
            BitBlt(hdcWindow, 0, 0, screenWidth, screenHeight, hdcMem, 0, 0, SRCCOPY);
            Sleep(16);
            continue;
        }
        
        if (GetAsyncKeyState('A') & 0x8000) playerAngle -= rotSpeed;
        if (GetAsyncKeyState('D') & 0x8000) playerAngle += rotSpeed;
        
        double nextX = playerX;
        double nextY = playerY;
        if (GetAsyncKeyState('W') & 0x8000) {
            nextX += cos(playerAngle) * moveSpeed;
            nextY += sin(playerAngle) * moveSpeed;
        }
        if (GetAsyncKeyState('S') & 0x8000) {
            nextX -= cos(playerAngle) * moveSpeed;
            nextY -= sin(playerAngle) * moveSpeed;
        }
        
        if (map[(int)nextY * MAP_WIDTH + (int)nextX] != '#') {
            playerX = nextX;
            playerY = nextY;
        }
        
        double objectiveDistance = std::sqrt(
            (playerX - objectiveX) * (playerX - objectiveX) +
            (playerY - objectiveY) * (playerY - objectiveY));
        
        if (objectiveDistance < 0.8) {
            objectiveReached = true;
            continue;
        }
        
        PatBlt(hdcMem, 0, 0, screenWidth, screenHeight, BLACKNESS);
        
        int rayCount = 160;
        int columnWidth = (screenWidth / rayCount) + 1;
        for (int x = 0; x < rayCount; x++) {
            double rayAngle = (playerAngle - fov / 2.0) + ((double)x / (double)rayCount) * fov;
            double distanceToWall = 0.0;
            bool hitWall = false;
            double eyeX = cos(rayAngle);
            double eyeY = sin(rayAngle);
            
            while (!hitWall && distanceToWall < maxDepth) {
                distanceToWall += 0.05;
                int testX = (int)(playerX + eyeX * distanceToWall);
                int testY = (int)(playerY + eyeY * distanceToWall);
                
                if (testX < 0 || testX >= MAP_WIDTH || testY < 0 || testY >= MAP_HEIGHT) {
                    hitWall = true;
                    distanceToWall = maxDepth;
                } else if (map[testY * MAP_WIDTH + testX] == '#') {
                    hitWall = true;
                }
            }
            
            distanceToWall *= cos(rayAngle - playerAngle);
            int wallHeight = (int)((double)screenHeight / (distanceToWall < 0.1 ? 0.1 : distanceToWall));
            if (wallHeight > screenHeight) wallHeight = screenHeight;
            
            int ceiling = (screenHeight / 2) - (wallHeight / 2);
            int floor = (screenHeight / 2) + (wallHeight / 2);
            
            int redShade = (int)(255.0 * (1.0 - (distanceToWall / maxDepth)));
            if (redShade < 0) redShade = 0;
            
            COLORREF wallColor = RGB(redShade, 8, 0);
            HBRUSH hWallBrush = CreateSolidBrush(wallColor);
            RECT wallSlice = { x * (columnWidth - 1), ceiling, x * (columnWidth - 1) + columnWidth, floor };
            FillRect(hdcMem, &wallSlice, hWallBrush);
            DeleteObject(hWallBrush);
        }
        
        SetBkMode(hdcMem, TRANSPARENT);
        SetTextColor(hdcMem, RGB(255, 255, 255));
        wchar_t objectiveText[128];
        double targetAngle = std::atan2(objectiveY - playerY, objectiveX - playerX);
        double bearing = targetAngle - playerAngle;
        while (bearing > PI) bearing -= 2.0 * PI;
        while (bearing < -PI) bearing += 2.0 * PI;
        
        const wchar_t* direction = bearing > 0.15 ? L"TURN RIGHT" : bearing < -0.15 ? L"TURN LEFT" : L"AHEAD";
        swprintf_s(objectiveText, L"OBJECTIVE: REACH THE BEACON %s DISTANCE: %.1f", direction, objectiveDistance);
        TextOutW(hdcMem, 18, 18, objectiveText, static_cast<int>(wcslen(objectiveText)));
        
        DrawScrollingCredit(hdcMem, creditFont, screenWidth, screenHeight, creditPosition);
        BitBlt(hdcWindow, 0, 0, screenWidth, screenHeight, hdcMem, 0, 0, SRCCOPY);
        Sleep(1);
    }
    
    SelectObject(hdcMem, hOldBm);
    DeleteObject(creditFont);
    DeleteObject(hbmMem);
    DeleteDC(hdcMem);
    ReleaseDC(hwnd, hdcWindow);
    DestroyWindow(hwnd);
    Gdiplus::GdiplusShutdown(gdiplusToken);
    
    return 0;
}