#include "common.hpp"

// Основная логика, работающая в отдельном потоке после загрузки DLL
DWORD WINAPI MainThread(LPVOID lpParam) {
    // Выделяем консоль для удобной отладки
    AllocConsole();
    FILE* fDummy;
    freopen_s(&fDummy, "CONOUT$", "w", stdout);

    std::cout << "[+] DLL успешно загружена в процесс!" << std::endl;
    std::cout << "[+] Для выгрузки DLL зажмите клавишу [END / ENDE]" << std::endl;

    // Простейший цикл жизни DLL
    while (!GetAsyncKeyState(VK_END)) {
        // Здесь будет твоя основная логика работы DLL
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << "[-] Запущен процесс выгрузки DLL..." << std::endl;

    // Освобождаем консоль и закрываем потоки
    if (fDummy) {
        fclose(fDummy);
    }
    FreeConsole();

    // Выгружаем саму себя из памяти процесса и завершаем поток
    FreeLibraryAndExitThread((HMODULE)lpParam, 0);
    return 0;
}

// Точка входа для динамической библиотеки (DLL)
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        // Отключаем вызовы DLL_THREAD_ATTACH и DLL_THREAD_DETACH для оптимизации
        DisableThreadLibraryCalls(hModule);
        
        // Создаем отдельный поток, чтобы не блокировать основной поток загрузчика
        if (HANDLE hThread = CreateThread(nullptr, 0, MainThread, hModule, 0, nullptr)) {
            CloseHandle(hThread);
        }
        break;
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
