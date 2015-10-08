add_executable(metadata_to_csv metadata_to_csv.cpp)

# Why the extra -mt? Is there a cleaner way?
if("${CMAKE_SYSTEM}" MATCHES "Darwin")
   target_link_libraries(metadata_to_csv ${GNURADIO_ALL_LIBRARIES} ${UHD_LIBRARIES} boost_thread-mt boost_system-mt boost_program_options-mt)
else()
   target_link_libraries(metadata_to_csv ${GNURADIO_ALL_LIBRARIES} ${UHD_LIBRARIES} boost_thread boost_system boost_program_options)
endif()

install(
   FILES ${CMAKE_BINARY_DIR}/apps/matlab/metadata_to_csv
   PERMISSIONS OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
   DESTINATION ${GR_RUNTIME_DIR}
)
