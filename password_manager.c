#include <furi.h>
#include <furi_hal.h>
#include <gui/gui.h>
#include <input/input.h>
#include <stdlib.h>
#include <storage/storage.h>
#include <notification/notification.h>
#include <notification/notification_messages.h>

#include "password_storage.h"
#include "password_view.h"

#define TAG "PasswordManager"
#define PASSWORDS_FILE_PATH "/ext/passwords/passwords.txt"

// Definice scén
enum {
    SceneMain,
    SceneList,
    SceneView,
    SceneEdit,
    SceneHelp,
    SceneCount
};

// Definice událostí
typedef enum {
    EventTypeTick,
    EventTypeKey,
    EventTypeBack,
} EventType;

typedef struct {
    EventType type;
    InputEvent input;
} PasswordManagerEvent;

// Struktura aplikace
typedef struct {
    // Stav
    int current_scene;
    int selected_index;
    bool is_editing;
    
    // Data
    PasswordList password_list;
    char name_buffer[NAME_MAX_LENGTH];
    char password_buffer[PASSWORD_MAX_LENGTH];
    
    // GUI
    ViewPort* view_port;
    Gui* gui;
    NotificationApp* notifications;
    
    // Události
    FuriMessageQueue* event_queue;
} PasswordManager;

// Prototypy funkcí
static void password_manager_render_callback(Canvas* canvas, void* ctx);
static void password_manager_input_callback(InputEvent* input_event, void* ctx);
static void password_manager_draw_main_scene(Canvas* canvas, PasswordManager* app);
static void password_manager_draw_list_scene(Canvas* canvas, PasswordManager* app);
static void password_manager_draw_view_scene(Canvas* canvas, PasswordManager* app);
static void password_manager_draw_edit_scene(Canvas* canvas, PasswordManager* app);
static void password_manager_draw_help_scene(Canvas* canvas, PasswordManager* app);

// Inicializace aplikace
static PasswordManager* password_manager_alloc() {
    PasswordManager* app = malloc(sizeof(PasswordManager));
    
    // Inicializace dat
    password_list_init(&app->password_list);
    password_list_load(&app->password_list, PASSWORDS_FILE_PATH);
    
    app->current_scene = SceneMain;
    app->selected_index = 0;
    app->is_editing = false;
    
    // Inicializace GUI
    app->view_port = view_port_alloc();
    app->gui = furi_record_open(RECORD_GUI);
    app->notifications = furi_record_open(RECORD_NOTIFICATION);
    
    view_port_draw_callback_set(app->view_port, password_manager_render_callback, app);
    view_port_input_callback_set(app->view_port, password_manager_input_callback, app);
    
    // Přidání view_port do GUI
    gui_add_view_port(app->gui, app->view_port, GuiLayerFullscreen);
    
    // Inicializace fronty událostí
    app->event_queue = furi_message_queue_alloc(8, sizeof(PasswordManagerEvent));
    
    return app;
}

// Uvolnění aplikace
static void password_manager_free(PasswordManager* app) {
    // Uložení hesel
    password_list_save(&app->password_list, PASSWORDS_FILE_PATH);
    
    // Uvolnění GUI
    view_port_enabled_set(app->view_port, false);
    gui_remove_view_port(app->gui, app->view_port);
    view_port_free(app->view_port);
    
    // Uvolnění záznamů
    furi_record_close(RECORD_GUI);
    furi_record_close(RECORD_NOTIFICATION);
    
    // Uvolnění fronty událostí
    furi_message_queue_free(app->event_queue);
    
    // Uvolnění paměti
    free(app);
}

// Callback pro vykreslení
static void password_manager_render_callback(Canvas* canvas, void* ctx) {
    PasswordManager* app = ctx;
    
    canvas_clear(canvas);
    canvas_set_font(canvas, FontPrimary);
    
    // Vykreslení aktuální scény
    switch(app->current_scene) {
        case SceneMain:
            password_manager_draw_main_scene(canvas, app);
            break;
        case SceneList:
            password_manager_draw_list_scene(canvas, app);
            break;
        case SceneView:
            password_manager_draw_view_scene(canvas, app);
            break;
        case SceneEdit:
            password_manager_draw_edit_scene(canvas, app);
            break;
        case SceneHelp:
            password_manager_draw_help_scene(canvas, app);
            break;
        default:
            break;
    }
}

// Callback pro vstup
static void password_manager_input_callback(InputEvent* input_event, void* ctx) {
    PasswordManager* app = ctx;
    
    // Vytvoření události
    PasswordManagerEvent event = {
        .type = EventTypeKey,
        .input = *input_event
    };
    
    // Odeslání události do fronty
    furi_message_queue_put(app->event_queue, &event, FuriWaitForever);
}

// Vykreslení hlavní scény
static void password_manager_draw_main_scene(Canvas* canvas, PasswordManager* app) {
    canvas_draw_str(canvas, 2, 10, "Password Manager");
    canvas_draw_str(canvas, 2, 22, "Počet hesel: ");
    canvas_draw_str_aligned(
        canvas, 
        90, 
        22, 
        AlignLeft, 
        AlignTop, 
        furi_string_get_cstr(furi_string_alloc_printf("%lu", app->password_list.count))
    );
    
    canvas_draw_str(canvas, 2, 34, "Nahoru/Dolů: Procházet");
    canvas_draw_str(canvas, 2, 46, "OK: Seznam hesel");
    canvas_draw_str(canvas, 2, 58, "Zpět: Ukončit");
}

// Vykreslení scény seznamu
static void password_manager_draw_list_scene(Canvas* canvas, PasswordManager* app) {
    canvas_draw_str(canvas, 2, 10, "Seznam hesel");
    
    if(app->password_list.count == 0) {
        canvas_draw_str(canvas, 2, 22, "Žádná hesla");
        canvas_draw_str(canvas, 2, 34, "Dlouhý stisk OK: Přidat");
        return;
    }
    
    // Zobrazení seznamu hesel
    int start_index = 0;
    if(app->selected_index > 2) {
        start_index = app->selected_index - 2;
    }
    
    for(int i = 0; i < 4 && (i + start_index) < app->password_list.count; i++) {
        int y = 22 + i * 10;
        
        // Zvýraznění vybrané položky
        if(i + start_index == app->selected_index) {
            canvas_draw_str(canvas, 0, y, ">");
        }
        
        canvas_draw_str(canvas, 10, y, app->password_list.items[i + start_index].name);
    }
    
    canvas_draw_str(canvas, 2, 58, "OK: Zobrazit, Dlouhý: Přidat");
}

// Vykreslení scény zobrazení hesla
static void password_manager_draw_view_scene(Canvas* canvas, PasswordManager* app) {
    if(app->selected_index >= app->password_list.count) {
        canvas_draw_str(canvas, 2, 10, "Chyba: Neplatný index");
        return;
    }
    
    PasswordItem* item = &app->password_list.items[app->selected_index];
    
    canvas_draw_str(canvas, 2, 10, "Heslo:");
    canvas_draw_str(canvas, 2, 22, item->name);
    
    // Zobrazení hesla
    canvas_draw_str(canvas, 2, 34, "Heslo:");
    canvas_draw_str(canvas, 2, 46, item->password);
    
    canvas_draw_str(canvas, 2, 58, "OK: Odeslat, Dlouhý: Smazat");
}

// Vykreslení scény úpravy
static void password_manager_draw_edit_scene(Canvas* canvas, PasswordManager* app) {
    canvas_draw_str(canvas, 2, 10, app->is_editing ? "Úprava hesla" : "Nové heslo");
    
    canvas_draw_str(canvas, 2, 22, "Název:");
    canvas_draw_str(canvas, 50, 22, app->name_buffer);
    
    canvas_draw_str(canvas, 2, 34, "Heslo:");
    canvas_draw_str(canvas, 50, 34, app->password_buffer);
    
    // Zvýraznění aktivního pole
    canvas_draw_str(canvas, 40, app->is_editing ? 34 : 22, ">");
    
    canvas_draw_str(canvas, 2, 58, "OK: Potvrdit, Zpět: Zrušit");
}

// Vykreslení scény nápovědy
static void password_manager_draw_help_scene(Canvas* canvas, PasswordManager* app) {
    UNUSED(app);
    
    canvas_draw_str(canvas, 2, 10, "Nápověda");
    canvas_draw_str(canvas, 2, 22, "Správce hesel pro Flipper Zero");
    canvas_draw_str(canvas, 2, 34, "Autor: Augment Agent");
    canvas_draw_str(canvas, 2, 46, "Verze: 1.0");
    canvas_draw_str(canvas, 2, 58, "Zpět: Návrat");
}

// Zpracování událostí
static void password_manager_process_event(PasswordManager* app, PasswordManagerEvent* event) {
    if(event->type == EventTypeKey) {
        // Zpracování klávesových událostí
        if(event->input.type == InputTypeShort) {
            switch(event->input.key) {
                case InputKeyBack:
                    // Zpět
                    if(app->current_scene == SceneMain) {
                        // Ukončení aplikace
                        furi_message_queue_put(app->event_queue, &(PasswordManagerEvent){.type = EventTypeBack}, 0);
                    } else {
                        // Návrat na předchozí scénu
                        app->current_scene = SceneMain;
                    }
                    break;
                    
                case InputKeyUp:
                    // Nahoru
                    if(app->current_scene == SceneList && app->selected_index > 0) {
                        app->selected_index--;
                    }
                    break;
                    
                case InputKeyDown:
                    // Dolů
                    if(app->current_scene == SceneList && app->selected_index < app->password_list.count - 1) {
                        app->selected_index++;
                    }
                    break;
                    
                case InputKeyOk:
                    // OK
                    if(app->current_scene == SceneMain) {
                        // Přechod na seznam hesel
                        app->current_scene = SceneList;
                    } else if(app->current_scene == SceneList) {
                        // Přechod na zobrazení hesla
                        if(app->password_list.count > 0) {
                            app->current_scene = SceneView;
                        }
                    } else if(app->current_scene == SceneView) {
                        // Odeslání hesla
                        if(app->selected_index < app->password_list.count) {
                            password_send_as_keyboard(app->password_list.items[app->selected_index].password);
                            
                            // Notifikace o odeslání
                            notification_message(app->notifications, &sequence_blink_green_100);
                        }
                    }
                    break;
                    
                case InputKeyRight:
                    // Vpravo - přechod na nápovědu
                    if(app->current_scene == SceneMain) {
                        app->current_scene = SceneHelp;
                    }
                    break;
                    
                default:
                    break;
            }
        } else if(event->input.type == InputTypeLong) {
            // Dlouhý stisk
            switch(event->input.key) {
                case InputKeyOk:
                    if(app->current_scene == SceneList) {
                        // Přidání nového hesla
                        app->is_editing = false;
                        memset(app->name_buffer, 0, sizeof(app->name_buffer));
                        memset(app->password_buffer, 0, sizeof(app->password_buffer));
                        app->current_scene = SceneEdit;
                    } else if(app->current_scene == SceneView) {
                        // Smazání hesla
                        if(app->selected_index < app->password_list.count) {
                            password_list_remove(&app->password_list, app->selected_index);
                            password_list_save(&app->password_list, PASSWORDS_FILE_PATH);
                            
                            // Návrat na seznam
                            app->current_scene = SceneList;
                            
                            // Korekce indexu
                            if(app->selected_index >= app->password_list.count) {
                                app->selected_index = app->password_list.count > 0 ? app->password_list.count - 1 : 0;
                            }
                            
                            // Notifikace o smazání
                            notification_message(app->notifications, &sequence_blink_red_100);
                        }
                    } else if(app->current_scene == SceneEdit) {
                        // Přepnutí mezi názvem a heslem
                        app->is_editing = !app->is_editing;
                    }
                    break;
                    
                case InputKeyBack:
                    if(app->current_scene == SceneEdit) {
                        // Uložení hesla
                        if(strlen(app->name_buffer) > 0 && strlen(app->password_buffer) > 0) {
                            password_list_add(&app->password_list, app->name_buffer, app->password_buffer);
                            password_list_save(&app->password_list, PASSWORDS_FILE_PATH);
                            
                            // Návrat na seznam
                            app->current_scene = SceneList;
                            
                            // Výběr nově přidaného hesla
                            app->selected_index = app->password_list.count - 1;
                            
                            // Notifikace o přidání
                            notification_message(app->notifications, &sequence_blink_green_100);
                        }
                    }
                    break;
                    
                default:
                    break;
            }
        }
    } else if(event->type == EventTypeBack) {
        // Ukončení aplikace
        view_port_enabled_set(app->view_port, false);
    }
}

// Hlavní funkce aplikace
int32_t password_manager_app(void* p) {
    UNUSED(p);
    
    // Alokace aplikace
    PasswordManager* app = password_manager_alloc();
    
    // Hlavní smyčka
    PasswordManagerEvent event;
    bool running = true;
    
    while(running) {
        // Čekání na událost
        FuriStatus status = furi_message_queue_get(app->event_queue, &event, 100);
        
        if(status == FuriStatusOk) {
            // Zpracování události
            if(event.type == EventTypeBack) {
                running = false;
            } else {
                password_manager_process_event(app, &event);
            }
        }
        
        // Překreslení GUI
        view_port_update(app->view_port);
    }
    
    // Uvolnění aplikace
    password_manager_free(app);
    
    return 0;
}
