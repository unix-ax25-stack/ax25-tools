
installconf:
	@echo "**** WARNING - This will overwrite any existing files ****"
	@echo "**** WARNING - This will overwrite any existing files ****"
	@echo "**** WARNING - This will overwrite any existing files ****"
	@echo
	@echo Hit the return key to proceed, or ^C to exit.
	@read
	@for app in $(SUBDIRS); do $(MAKE) -C $$app installconf; done

SUBDIRS = ax25 hdlcutil kiss netrom rose tcpip user_call 
