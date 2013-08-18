PSP Bootloader
==============
PSP Bootloader is a homebrew for the Sony PSP wich allows you to boot kernel images, like "uClinux on PSP"
It provides a GRUB-inspired menu wich allows you to boot multiple kernel files without changing the config file.
The config file has a very simple syntax described below.

Config-file
===========
The config-file (pspboot.conf) holds all the information the bootloader needs to know to boot the kernel.
The syntax is as following:

	To specify a parameter:
		<name>=<value>
	where:
		<name> is the parameter name
		<value> is the value of the parameter. This can be a string or an integer, depending on the parameter name.
	
	To specify a menuentry:
		menuentry "<name of the entry>" 
		{
		<parameters>
		}
	where:
		<name of the entry> will be the title of the option in the menu.
		Between { and } the menuentry-parameters are specified (see below).

	Globally supported parameters:
		timeout:	Specify the amount of seconds to show the menu before boot. If a user presses any key on the D-Pad, the countdown is stopped.
				Setting timeout to -1 will always show the menu without the countdown. Setting timeout to 0 will skip the menu and will boot the first menuentry in the config file.
				The menu can always be accessed by holding the R-Trigger while the application starts up.
	Menuentry-parameters:
		kernel:	Specify the kernel image. this can be in the same directory as the config file or somewhere else, specified with a absolute path (ms0:/...). The image can be uncompressed, GZIP-compressed or BZIP2-compressed. \
				The bootloader will automatically detect the compression type.
		cmdline:	Specify the Linux kernel command line. See the Linux documentation for more info.
		baud:     if the bootloader was build with ENABLE_UART3=1, this variable sets the BAUD for the UART3. 

		
	

