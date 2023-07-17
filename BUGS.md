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
- sometimes sender calling `sendtoWait` just stalls. adding a watchdog could help, but root cause still not known yet