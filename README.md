# Install the yang using sysrepoctl -i
```
sysrepoctl -i o-ran-sync-state-change-noti-test.yang
```
# Compile the program to generate an exectuable 
```
 gcc -o sync-state-notifier sync-state-notifier.c $(pkg-config --cflags --libs libyang sysrepo)
```

# In a terminal run the program
```
./sync-state-notifier
```

### In another terminal run the configured netconf server and connect to this using netconf client and subscribe to the notification to receive the notification. 
