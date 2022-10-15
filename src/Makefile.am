bin_PROGRAMS = example
example_SOURCES = \
	application.cpp \
	application.hpp \
	configuration.cpp \
	configuration.hpp \
	date_utils.cpp \
	date_utils.hpp \
	debug_utils.cpp \
	debug_utils.hpp \
	example.cpp \
	file_utils.cpp \
	file_utils.hpp \
	get_options.cpp \
	get_options.hpp \
	http_request.cpp \
	http_request.hpp \
	http_request_factory.cpp \
	http_request_factory.hpp \
	http_request_handler.cpp \
	http_request_handler.hpp \
	http_response.cpp \
	http_response.hpp \
	logger.cpp \
	logger.hpp \
	pagination.cpp \
	pagination.hpp \
	pg_pool.cpp \
	pg_pool.hpp \
	session.cpp \
	session.hpp \
	socket.cpp \
	socket.hpp \
	uri_utils.cpp \
	uri_utils.hpp \
	uuid.cpp \
	uuid.hpp \
	worker.cpp \
	worker.hpp

check_PROGRAMS = test_uuid test_dao_helper test_date_utils test_file_utils \
	test_uri_utils test_debug_utils test_pagination test_pool
test_uuid_SOURCES = test_uuid.cpp
test_dao_helper_SOURCES = test_dao_helper.cpp date_utils.cpp date_utils.hpp \
	logger.cpp logger.hpp
test_date_utils_SOURCES = test_date_utils.cpp logger.cpp logger.hpp
test_file_utils_SOURCES = test_file_utils.cpp date_utils.cpp date_utils.hpp \
	logger.cpp logger.hpp
test_uri_utils_SOURCES = test_uri_utils.cpp
test_debug_utils_SOURCES = test_debug_utils.cpp
test_pagination_SOURCES = test_pagination.cpp logger.cpp logger.hpp \
	debug_utils.cpp debug_utils.hpp
test_pool_SOURCES = test_pool.cpp
