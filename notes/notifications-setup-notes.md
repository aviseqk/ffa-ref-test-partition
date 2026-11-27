SOURCE: [Arm DEN0077A Doc - Chapter 10]

### Interrupts setup for Notifications
*The Receiver and its scheduler have to configure support for handling interrupts used by Framework for notification signaling.*

- Framework uses `Schedule Receiver Interrupt (SRI)` to inform the receiver's scheduler
that receiver must run to handle a pending notification
	> the receiver's scheduler uses FFA_FEATURES with 0x2 ID to get description of the SRI
	[Check DEN0077A Sec10.4.1 and Figure 10.3 for SRI setup and handling within primary endpoint]
- Framework uses `Notification Pending Interrupt (NPI)` to inform the receiver that it
has a pending notification. 
	> NPI is a virtual interrupt
	> NPI is used if receiver is a VM under Hyp or a S-EL1 SP running under S-EL2 SPMC
	> The receiver gets the NPI's description by using FFA_FEATURES with 0x1 as ID
	[Check DEN0077A Sec10.4.1 and Figure 10.4 for NPI setup and handling within receiver endpoint]


### Notification Binding
- A receiver has to bind a non-framework notification to sender before sender can signal it
- `FFA_NOTIFICATION_BIND` interface is used to bind a notif
- A notification won't be signaled and pended unless it is bound to a Sender endpoint.
- The notification bitmap in which a said notification will be bound depends on the security
  state of the sender endpoint.
	> If the sender is a SP, SP notifications bitmap is used
	> If the sender is a VM, VM notifications bitmap is used
- A notification is signaled and then it has to be handled by some execution context or vCPU.
	> If the receiver endpoint has a specific execution context or vCPU on which 
	it will handle the said notification, it is `per-vCPU notifications`
	> If the receiver endpoint has no say in choosing the context, rather the 
	receiver's scheduler or partition manager chooses the context for the receiver endpoint,
	then it is `Global notifications`
[Check DEN0077A Sec10.4.2 for detailed guide on notification binding]


### Notification Signaling
- 





```text
NOTE: the `Rx-Buffer-Full` notification is a Framework Notification, it is no standard notification.
Framework Notifications are used by SPMC or partition managers to signal common events to an endpoint,
- An endpoint cannot and should not be able to signal the Framework notifications. 
- A partition manager can signal a Framework Notification in response to a standard FF-A ABI invocated by an endpoint
```


#### Development Note:
```text
The testing scenario in this SP regarding FF-A Notifications.

So, the two FF-A endpoints here is this SP, and then the Linux i.e. Normal World VM. Both of these endpoints can be either a
sender or a receiver, hence it is a two-way two-responsibility testing cycle.

The following tests also cover the whole lifecycle of Notification as per Arm, i.e. situations where a Notification ID is
- MASKED	=> Notification is not bound to any endpoint
- UNMASKED	=> Notification is bound to a Sender and ready to receive the signals
- PENDING	=> Notification is bound and also has been signaled by the sender

The testing sequence or the notification signaling sequence I implemented in this is:
1. "Unbound Sender Test" -> Linux tries to send a notification that is not bound to the SP
2. "Basic SET/GET" -> A single notification ID is bound by the sender endpoint and later retrieves by the receiver endpoint
3. "Unbind while PENDING" -> A notification has been signaled but not consumed by the receiver, and receiver tries to unbind it
4. "Basic GET clears PENDING" -> The receiver deliberately GETs the notification once again after consuming, to make sure it is cleared
5. "Unbind after GET" -> A happy case where after receiver has GET the notification and consumed, receiver tries to unbind it and hooray
6. "Multiple Notification IDs" -> SP signals multiple notification IDs together at once, and other endpoint i.e. Linux 
	should callback trigger callbacks associated with all said IDs
7. "Repeated SET" -> SP sets a single notification ID repeatedly while it remains pending, receiver should still read it as one signal
8. "Rebind" -> A notification being unbound and then rebound again by an endpoint
9. "Post-Rebind SET" -> Post rebind, a SET by the sender should exactly be the same as before

NOTES:
- For "repeated SETs", I needed a mechanism so that the receiver does not consume the bit between repeated calls, for that I used `delay_sri` parameter



```
