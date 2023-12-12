add_library(cred Cache.cpp Store.cpp CredentialHelper.cpp GitCredential.cpp)

target_link_libraries(cred conf git Qt5::Core)

set_target_properties(cred PROPERTIES AUTOMOC ON)
