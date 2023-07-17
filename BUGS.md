# BUGS

This page is made to list all bugs found

---

- calling `RHMeshManager_.sendtoWait` with no other receiver node active, gives a return of `RH_ROUTER_ERROR_NONE`. shouldn't it be any of the others?
    ```
    #define RH_ROUTER_ERROR_NO_ROUTE          2
    #define RH_ROUTER_ERROR_TIMEOUT           3
    #define RH_ROUTER_ERROR_NO_REPLY          4
    #define RH_ROUTER_ERROR_UNABLE_TO_DELIVER 5
    ```
- fail to flash concurrently multiple env profiles
    ```
    [platformio]
    default_envs = node-id-3, node-id-255
    ```
    it succeeds in flashing both ports iteratively, but fail to have different build flags, they both have this configuration
    ```
    build_flags = 
        -DSELF_ADDRESS=255
        -DTARGET_ADDRESS=3
    ```