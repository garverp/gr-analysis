add_executable(specrec specrec.cpp)
target_link_libraries(specrec ${GNURADIO_ALL_LIBRARIES} ${UHD_LIBRARIES} boost_thread boost_system boost_program_options)
                      
install(
   FILES ${CMAKE_BINARY_DIRECTORY}/apps/record/specrec
    DESTINATION ${GR_RUNTIME_DIR}
    #PERMISSIONS OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
)
