cmake_minimum_required(VERSION 3.1)

set(CMAKE_CXX_STANDARD 11)
project(observer_agent VERSION 1.0 LANGUAGES CXX)
set(GRPC_PATH ${CMAKE_CURRENT_SOURCE_DIR}/grpc_comm)
add_custom_target(grpc_demo ALL
	COMMAND make -C ${GRPC_PATH})

add_executable(secDetectord grpc_comm/client.cpp grpc_comm/comm_server.cpp service/main.cpp service/ringbuffer.cpp grpc_comm/comm_api.pb.o grpc_comm/comm_api.grpc.pb.o)
target_include_directories(secDetectord PUBLIC service grpc_comm)
# target_link_libraries(secDetectord protobuf grpc++ grpc)
set(SDK_LINK_FLAGS " -lprotobuf -lpthread -lgrpc++ -lgrpc -laddress_sorting -lre2 -lupb -lcares -lz -lgpr -lssl -lcrypto -labsl_hash -labsl_city -labsl_low_level_hash -labsl_raw_hash_set -labsl_hashtablez_sampler -labsl_statusor -labsl_status -labsl_cord -labsl_cordz_info -labsl_cord_internal -labsl_cordz_functions -labsl_exponential_biased -labsl_cordz_handle -labsl_bad_optional_access -labsl_strerror -labsl_str_format_internal -labsl_synchronization -labsl_graphcycles_internal -labsl_stacktrace -labsl_symbolize -labsl_debugging_internal -labsl_demangle_internal -labsl_malloc_internal -labsl_time -labsl_civil_time -labsl_strings -labsl_strings_internal -lrt -labsl_base -labsl_spinlock_wait -labsl_int128 -labsl_throw_delegate -labsl_time_zone -labsl_bad_variant_access -labsl_raw_logging_internal -labsl_log_severity")
set_target_properties(secDetectord PROPERTIES LINK_FLAGS "${SDK_LINK_FLAGS}")
