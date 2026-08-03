# my_dll_project — PastAware (internal, CS2)

Internal DLL для CS2: меню рисуется прямо в игре (D3D11 overlay), без
консоли и внешних окон.

## Что изменилось (по сравнению со старой версией)

- **Убрана консоль** (`AllocConsole`/`freopen`/вся отладочная печать) —
  ничего не «пишется непонятно», DLL не светится окном.
- **Убран «внешний» режим**: старого Win32-окна с GDI-меню больше нет.
  Вместо этого — классический internal-хук:
  - `Present` / `ResizeBuffers` IDXGISwapChain (индексы vtable 8/13,
    как в velocity) через dummy-swap-chain + патч vtable;
  - подмена `WndProc` окна игры (класс `SDL_app`) для ввода;
  - перехват мыши через `CInputSystem::SetRelativeMouseMode`
    (интерфейс `InputSystemVersion001` из свежих оффсетов
    `offset-pattern/interfaces.hpp`), чтобы курсор был свободен в меню.
- **Меню взято из velocity** (`исходники/velocity/...`) и адаптировано:
  - перенесён весь UI-движок **xdraw + xui** (тот же внешний вид: тёмная
    тема, сайдбар с иконками, плавающая «таблетка» сабтабов, поиск по
    настройкам, переключатели тем в сайдбаре, watermark, индикатор
    активных биндов);
  - переименовано в **PastAware** (логотип, watermark «pastaware»);
  - настройки сделаны самодостаточными (`src/settings.hpp`) — без
    привязки к фичам velocity;
  - вкладки: Rage, Legit, Player, World, Skins, Misc, Config
    (сабтабы как в velocity);
  - правый клик по чекбоксу — привязка клавиши; поиск — по иконке лупы.

## Структура

```
src/
  pch.hpp          общий заголовок
  dllmain.cpp      точка входа (без консоли), END — выгрузка
  hooks.hpp/.cpp   хуки D3D11 Present/ResizeBuffers + WndProc + мышь
  context.hpp/.cpp рендер-контекст (device/RTV/кадр)
  menu.hpp/.cpp    меню (порт из velocity)
  settings.hpp     настройки (чекбоксы/слайдеры/бинды)
  menu_svgs.hpp    SVG-иконки меню (из velocity)
  menu_assets.hpp  встроенный PNG-аватар (из velocity)
  memory.hpp       мелкие хелперы чтения/vfunc/интерфейсов
external/xdraw/    UI-движок velocity (xdraw + xui + freetype + шрифты)
```

## Сборка

- Visual Studio 2022, конфигурация **Release | x64** (или Debug | x64),
  Windows SDK 10.x.
- Требуются: Windows SDK (D3D11/DXGI/WIC), FreeType уже включён
  (`external/xdraw/dependencies/freetype/x/freetype.lib`, x64).
- Открыть `my_dll_project.sln` → Build. Результат:
  `bin\Release\my_dll_project.dll`.

## Использование

1. Загрузить DLL в процесс cs2.exe (любой инжектор).
2. Меню открыто сразу. **INSERT** — показать/скрыть, **END** — выгрузить
   (или кнопка «unload dll» во вкладке Config).
3. Оффсеты для игры лежат в `../offset-pattern` (cs2-dumper, билд 14173,
   2026-07-29) — при обновлении игры обновить их.

## Примечания

- Индексы vfunc (Present=8, ResizeBuffers=13, SetRelativeMouseMode=76)
  взяты из исходников velocity.
- Код меню — копия velocity (лицензия/атрибуция см. `../исходники/velocity/README.md`).
- Дальнейшие фичи (aimbot/ESP и т.д.) цепляются на готовый каркас: читайте
  `settings.hpp` из хуков и рисуйте через `xdraw::get()` в `context.cpp`.
