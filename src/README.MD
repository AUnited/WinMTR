WinMTR - GUI MTR implementation for Windows
=============
Thank you for downloading WinMTR v1.2!

## About

WinMTR is a free MS Windows visual application that combines the functionality of the traceroute and ping in a single network diagnostic tool. WinMTR is Open Source Software, maintained by Appnor MSP, a fully managed hosting & cloud provider.

It was started in 2000 by Vasile Laurentiu Stanimir  as a clone for the popular Matt’s Traceroute (hence MTR) Linux/UNIX utility. 
Read more about WinMTR’s history: http://winmtr.net/history/

## License & Redistribution

WinMTR is offered as Open Source Software under GPL v2. 
Read more about the licensing conditions: http://www.gnu.org/licenses/gpl-2.0.html
Download the code from: http://sourceforge.net/projects/winmtr/

## Installation

You will get an executable.
Just extract the WinMTR.exe for your platform (x86 or ARM, 32 or 64 bit) and click to run it.
If you don’t know what version you need, just click on files and see which one works.
As you can see, WinMTR requires no other installation effort.

Trick: You can copy the WinMTR.exe file in Windows/System32, so it’s accessible via the command line – cmd!

## Usage

**Visual:**

- Start WinMTR.
- Write the name or IP of the host (e.g. google.com)
- Press the Options button to configure ping size, maximum hops and ping interval (the defaults are OK).
- Push the Start button and wait.
- Copy or export the results in text or HTML format. Useful if you want to document or file a complaint with your ISP.
- Click on Clear History to remove the hosts you have previously traced.

**Command line:**

Run `winmtr.exe --help` to see what are the options 

Run `winmtr hostname` (e.g. winmtr www.yahoo.com)

## Troubleshooting

1. I type in the address and nothing happens.

Usually this has to do with lame anti-virus or firewall applications. Stop all that when doing debugging or when using WinMTR. Or, configure them properly.

2. I get an error saying the program cannot be executed.

You are running the 64 bit version on a 32-bit platform. Try the WinMTR.exe stored in the WinMTR_x32 folder.

3. I get an error unspecified here.

Please report it to us to make sure it’s not a bug in the application.

### Bug Reports

Let us know if you identify bugs. Make sure you mention the WinMTR version. Also, we need as much info as possible about your Operating System and current setup. 
Before submitting a bug make sure it is not something related to your own specific configurations (e.g. anti-viruses, firewalls). 

### Feature requests

If you need some functionality from which others can also benefit, please let us know. We will try to integrate your request on our future releases.
Specific features can be implemented on request under a commercial support agreement. Costs depend on complexity and timing. Contact us for a custom quotation. 
If you are a developer planning to extend the current open source code, please let us know, so we can integrate it in the official tree


## Contact

Use the form available at https://sctools.cyou or write to us contact AT sctools DOT cyou

See License.md to read the credits
-