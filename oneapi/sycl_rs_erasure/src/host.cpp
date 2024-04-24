// This is just a wrapper file, selecting the proper host source file

#ifdef MULTI_ERASURE_SIMPLE
    #pragma message "[INFO] Importing host_multi_erasure.cpp"
    #include "host_multi_erasure.cpp"
#else // ! MULTI_ERASURE_SIMPLE
    #pragma message "[INFO] Importing host_one_erasure.cpp"
    #include "host_one_erasure.cpp"
#endif // ! MULTI_ERASURE_SIMPLE
