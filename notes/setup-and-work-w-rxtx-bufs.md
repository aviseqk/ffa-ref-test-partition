# Setting Up and Working with Rx_Tx Buffers for Indirect Messaging in our SP <-> Linux FF-A Instance 

### I_QFCQH rule in Arm's DEN0077A document states the following:
```text
Setup of a buffer pair is done by a physical endpoint through one of the following mechanisms:
   • An endpoint allocates the buffer pair and uses FFA_RXTX_MAP ABI (see 13.6 FFA_RXTX_MAP) to map
      it in the partition manager’s translation regime.
   • The endpoint requests buffer allocation in its manifest by specifying their base addresses (as IPAs or VAs)
      and size. The partition manager maps the buffer pair in the stage of translation regime it manages on behalf
      of the endpoint and its own translation regime.
```





