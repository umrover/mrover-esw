# STM32Cube*

## About

STM32CubeMX and STM32 CubeCLT allow the user to write, compile, and flash code to the STM32 microcontroller!
STM32CubeCLT contains the GCC compiler and GDB debugging tool for firmware compilation and debugging, and
STM32CubeMX contains the interface for configuring the microcontroller and project environment, and provides
a very powerful interface for automatic code generation, allowing the user to initialize an entire module with
a few clicks of a button, and have that code show up automatically in the main file.

## Downloading and Installing CubeMX

### Download CubeMX

1. Go to the CubeMX [download page](https://www.st.com/en/development-tools/stm32cubemx.html) and scroll down to "Get Software."
   ![get cubemx](get-cubemx.webp)

2. **_For this walkthrough, we will be using the Linux installer for UBUNTU USERS._**
   Click on "Select Version", then select the **newest version** for the **_correct OS you are running!!!_**

3. Click on Accept for the License Agreement pop up.

4. You will then be prompted to log in, create an account, or continue as a guest. You will need a
   MyST account in the future, so it is best to create one now. You may use any email address.

5. After logging in and being brought back to the download page, the download should start automatically.
   Scroll down and select the correct version from "Get Software" if it does not.

### Install CubeMX

**Note**: This install guide is for **Linux**. Please make sure to install the correct version
for the OS you are running.

1\. Open a new terminal ([guide](https://www.howtogeek.com/686955/how-to-launch-a-terminal-window-on-ubuntu-linux/)).

2\. The downloaded installer zip file should be in your `Downloads` folder. You can navigate to this
folder by entering the following command in your terminal:

```sh
cd ~/Downloads
```

3\. You should now be able to see the installer `.zip` file, if you run the following command:

```sh
ls
```

4\. To unzip the installer, run the following command (this may take a second):

```sh
unzip <zip_file_name>
```

For example:

```sh
unzip stm32cubemx-lin-v6-16-0.zip
```

5\. You should now be able to see the installer executable along with `Readme.html` and a Java runtime by running the following command:

```sh
ls
```

6\. Run the installer by running the following (you may be prompted to enter your password):

```sh
sudo ./SetupSTM32CubeMX-<version>
```

For example:

```sh
sudo ./SetupSTM32CubeMX-6.16.0
```

7\. In the UI that opens, select `Next` and accept the License and Terms of Use.

8\. Select an appropriate installation directory (we recommend `/usr/local/STMicroelectronics/STM32Cube/STM32CubeMX`)

9\. Once the installation completes, select `Done`.

10\. To clean the installation, remove all files in `~/Downloads` from the installer with the following:

```sh
rm -rf jre Readme.html SetupSTM32CubeMX* stm32cubemx-lin-*.zip
```

11\. To use STM32CubeMX from the command line, it must be first added to the system path. To do this, we will use `vi` ([guide](https://opensource.com/article/19/3/getting-started-vim)). Run the following command:

```sh
sudo vi /etc/environment
```

And append your installation directory (for example, `/usr/local/STMicroelectronics/STM32Cube/STM32CubeMX`) to `PATH`, separated from the other paths by a `:`.

12\. Reboot your machine, open a terminal, and verify that the following command can find STM32CubeMX (it should output the path used in step 11):

```sh
which STM32CubeMX
```

## Downloading and Installing CubeCLT

### Download CubeCLT

1. Go to the CubeCLT [download page](https://www.st.com/en/development-tools/stm32cubeclt.html) and scroll down to "Get Software."
   ![get cubeclt](get-cubeclt.webp)

2. **_For this walkthrough, we will be using the Debian Linux installer for UBUNTU USERS._**
   Click on "Select Version", then select the **newest version** for the **_correct OS you are running!!!_**

3. Click on Accept for the License Agreement pop up.

4. You will then be prompted to log in, create an account, or continue as a guest. You will need a
   MyST account in the future, so it is best to create one now. You may use any email address.

5. After logging in and being brought back to the download page, the download should start automatically.
   Scroll down and select the correct version from "Get Software" if it does not.

### Install CubeCLT

**Note**: This install guide is for **Debian Linux**. Please make sure to install the correct version
for the OS you are running.

1\. Open a new terminal ([guide](https://www.howtogeek.com/686955/how-to-launch-a-terminal-window-on-ubuntu-linux/)).

2\. The downloaded installer zip file should be in your `Downloads` folder. You can navigate to this
folder by entering the following command in your terminal:

```sh
cd ~/Downloads
```

3\. You should now be able to see the installer `.zip` file, if you run the following command:

```sh
ls
```

4\. To unzip the installer, run the following command (this may take a second):

```sh
unzip <zip_file_name>
```

For example:

```sh
unzip st-stm32cubeclt_1.20.0_26822_20251117_1245_amd64.deb_bundle.sh.zip
```

5\. You should now be able to see the installer script by running the following command:

```sh
ls
```

6\. Run the installer by running the following (you may be prompted to enter your password):

```sh
sudo chomd +x <script>
sudo ./<script>
```

For example:

```sh
sudo chmod +x st-stm32cubeclt_1.20.0_26822_20251117_1245_amd64.deb_bundle.sh
sudo ./st-stm32cubeclt_1.20.0_26822_20251117_1245_amd64.deb_bundle.sh
```

7\. Accept the License Agreement (`y` and then `Enter`).

8\. To clean the installation, remove all files in `~/Downloads` from the installer with the following:

```sh
rm st-stm32cubeclt*
```

9\. To be able to use the various STM32CubeCLT components, add them all to path as was done with CubeMX.

The install path to STM32CubeCLT is `/opt/st/`, but each tool included has its own `bin` directory that must be added
to the path independently.

## Creating a New Project

This quick guide will teach you how to make a new project for your STM32G431RB Nucleo board that you
will be developing on.

### Prerequisites

-   STM32CubeIDE [installed](../stm32cubeide/index.md)

### Guide

Open STM32CubeIDE, select the desired workspace, and click `Launch` (the default is fine for now).

![image](https://user-images.githubusercontent.com/71603173/186999707-e8a45808-e55b-4859-a797-41e1fe225b05.png)

In the top left, go to `File`&#8594;`New`&#8594;`STM32 Project`.

![image](https://user-images.githubusercontent.com/71603173/186999816-2f289e9c-ebd2-4c3b-ae86-8a29d061ab75.png)

You may experience some lag after the previous step. Eventually, you will be prompted with the following window:

![image](https://user-images.githubusercontent.com/71603173/186999915-d8197e0a-cf00-43e0-a8ce-7c13c3039615.png)

At the top, select the `Board Selector` option.
Then on the left, in the text box next to `Commercial Part Number`, type `G431RB`.
There should only be one option in the `Boards List` Section, click that and press `Next`:

![board selector window with g431rb selected](board-select.webp)

Now you will be prompted with the following window:

![image](https://user-images.githubusercontent.com/71603173/187000137-4465eb7f-d7b6-4ec2-8986-57e7c16f9a14.png)

Here you can name your project whatever you want, but we can just call it "tutorial". Leave everything else as default and click Finish.

You will then be prompted with this window to configure this board. Make sure to uncheck all the
boxes by clicking `Unselect All`. Then select `OK`.

![board configuration window to select software components](board-config.webp)

It may also ask about changing perspective. Here, it comes down to preference, but you can just click `Yes`.

![image](https://user-images.githubusercontent.com/71603173/187000255-967961b9-7b45-4d6f-b0ca-4bb18dbb7210.png)

You should now see the following screen, which means that you have successfully created the project!
Notice the graphical interface of the chip on the right (which we may call the `.ioc` file) and the
files on the left that fall under the tutorial project (most importantly, `Src/main.c` and
`tutorial.ioc`). Cool things to know is the graphical interface is used to make changes to the chip
settings. If changes are made, then once the project is saved, code will be auto-generated for the
user. The `main.c` file is where you will want to write your code.

![image](https://user-images.githubusercontent.com/71603173/187000579-18856eed-e151-4cc7-a5d3-f419b2eff41e.png)

**Congratulations! You have successfully created a new project in CubeIDE!**

## Opening an Existing Project

This quick guide will teach you how to open an existing STM32 project in your STM32CubeIDE workspace.

### Prerequisites

-   STM32CubeIDE [installed](#install)
-   Any existing STM32 project downloaded

### Guide

Open up STM32CubeIDE.

Then, select the desired workspace that you want to open the project in and click `Launch`.

In the top left, go to `File`&#8594;`Open Projects from File System`.

Open the project directory in `Import source`.

Click `Finish`.

**Congratulations! You have successfully opened an existing project in CubeIDE!**

**_Note:_** Your project files will remain in the original directory that you opened
and any changes you make in STM32CubeIDE will reflect in the original directory.
All that you have done is added the project to the workspace, allowing you to
open it in STM32CubeIDE.
