add_executable(metadata_to_csv metadata_to_csv.cpp)
target_link_libraries(metadata_to_csv ${GNURADIO_ALL_LIBRARIES} ${UHD_LIBRARIES} boost_thread boost_system boost_program_options)
install(
    FILES metadata_to_csv
    #PERMISSIONS OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
    DESTINATION ${GR_RUNTIME_DIR}
)
