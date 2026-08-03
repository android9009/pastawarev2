#include "common.hpp"
#include "menu.hpp"

// Основная логика, работающая в отдельном потоке после загрузки DLL
DWORD WINAPI MainThread(LPVOID lpParam) {
    // Выделяем консоль для удобной отладки
    AllocConsole();
    FILE* fDummy = nullptr;
    freopen_s(&fDummy, "CONOUT$", "w", stdout);

    std::cout << "[+] DLL успешно загружена в процесс!" << std::endl;
    std::cout << "[+] INSERT — открыть/скрыть меню" << std::endl;
    std::cout << "[+] END — выгрузить DLL" << std::endl;

    if (!menu::start()) {
        std::cout << "[!] Не удалось запустить поток интерфейса." << std::endl;
    }

    // Меню обрабатывает сообщения в собственном потоке. Здесь остаётся
    // только горячая клавиша и контроль жизненного цикла DLL.
    while (!(GetAsyncKeyState(VK_END) & 0x8000)) {
        if (GetAsyncKeyState(VK_INSERT) & 1) {
            menu::toggle();
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    std::cout << "[-] Запущен процесс выгрузки DLL..." << std::endl;
    menu::stop();

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
