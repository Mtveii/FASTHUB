add_executable(fasthub_app
    main.cpp
    app.rc
)
target_link_libraries(fasthub_app PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    core
    ui
    infrastructure
    database
    network
    workspace
)
set_target_properties(fasthub_app PROPERTIES OUTPUT_NAME "FastHub" WIN32_EXECUTABLE ON)
