add_library(ui STATIC
    main_window.cpp
    main_window.h
    workspace_widget.cpp
    workspace_widget.h
    workspace/top_bar.cpp
    workspace/top_bar.h
    workspace/icon_rail.cpp
    workspace/icon_rail.h
    workspace/file_list_toolbar.cpp
    workspace/file_list_toolbar.h
    workspace/tree_chevron_button.h
    workspace/calendar_widget.cpp
    workspace/calendar_widget.h
    workspace/canvas_widget.cpp
    workspace/canvas_widget.h
    workspace/canvas_scene.cpp
    workspace/canvas_scene.h
    workspace/canvas_view.cpp
    workspace/canvas_view.h
    graph_view.cpp
    graph_view.h
    setup_dialog.cpp
    setup_dialog.h
    qr_login_dialog.cpp
    qr_login_dialog.h
    theme_manager.cpp
    theme_manager.h
    appearance_widget.cpp
    appearance_widget.h
    markdown_renderer.cpp
    markdown_renderer.h
    icon_util.cpp
    icon_util.h
    web_embed_widget.cpp
    web_embed_widget.h
    interface_settings.cpp
    interface_settings.h
    context_broker.cpp
    context_broker.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../assets/resources.qrc
    telegram/telegram_widget.cpp
    telegram/telegram_widget.h
    telegram/telegram_qr_login_dialog.cpp
    telegram/telegram_qr_login_dialog.h
    telegram/ayugram_embed_widget.cpp
    telegram/ayugram_embed_widget.h
    telegram/embed_types.h
    telegram/ayu_geometry_sync.cpp
    telegram/ayu_geometry_sync.h
    telegram/ayu_window_tracker.cpp
    telegram/ayu_window_tracker.h
    telegram/ayu_process_launcher.cpp
    telegram/ayu_process_launcher.h
    search/search_dialog.cpp
    search/search_dialog.h
    ai/ai_widget.cpp
    ai/ai_widget.h
    steam/steam_widget.cpp
    steam/steam_widget.h
    steam/steam_settings_dialog.cpp
    steam/steam_settings_dialog.h
    discord/discord_widget.cpp
    discord/discord_widget.h
    discord/discord_login_dialog.cpp
    discord/discord_login_dialog.h
    plugins/plugins_widget.cpp
    plugins/plugins_widget.h
    developer/developer_dialog.cpp
    developer/developer_dialog.h
)
target_include_directories(ui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ui PRIVATE
    Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Svg Qt6::WebEngineWidgets Qt6::Concurrent
    core workspace database infrastructure
    telegram_provider sync search ai_provider steam_provider discord_provider plugins
    qrcodegen md4c
    dwmapi
)

