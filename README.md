# Visual-Moon-Phase-with-stats-and-Sun-times
This is mostly a visualization of the moon phase and a tutorial on invisible-color sprite usage.

See the pictures, below, for a better idea what this does.  It was created as a simple little moon phase display.  It then got a moving starfield behind the moon.  It then got a little 2-button menu (not using a library).  The moon is inserted with an invisible color to cut off the four corners of the jpg then that is put onto another sprite that has a randomly created, slowly scrolling starfield on it. There are multiple sizes and color (pale blue, pale red, white and grey) of stars to make it look somewhat realistic and they scroll to the left as new stars come on from the right.

I did not do rotation because it very much depends on where you are and when you are as to what rotation you will see.  I may put it in some day.  The code to do the rotation is there but commented.  If you want to try it, go ahead.  The program gets the time of day and uses DST rules for any place on earth.  The moon can be turned on and off.  I just did that to see the entire starfield.  It also has a 3-item menu that will show you the next four major phases of the moon and sunrise and set times for your location and time.  You can turn the moon on and off with that menu, too.  Let me see someone do that with the real moon!!

Now, you might notice that the rise time in the picture, below, is yellow.  That's rarer than white.  It can also be red and that's even more rare.  It is normally white.  Here's what the colors mean.  If the Rise time happens tomorrow, I turn the label and time yellow to show that it is not going to happen today.  I guess I could go back to white after the time passes but it will go to white at midnight.  Maybe I will change that but I like it this way for now.  You can always change it yourself if you like.  When it is red, it not only means that the time is tomorrow but that it is also over 24 hours away so you won't be looking for a rise within the next hour.  You will have to hang around a bit longer than that.  This condition can happen for about 1 hour each day.  Otherwise, the time is in white.  I do the same color coding for the moon set time.  Please do not adjust your set.

Also, as expected, I place one of the 7 moon states on the bottom line of the display.  This alternates with brightness when you press a button (top for brighter, bottom for dimmer).  That brightness value is saved in Preferences and repeated tomorrow at the same hour.  To get the menu, press both buttons at once.  In the menu, press the bottom button to step through the menu (it is circular) and the top button to execute the selected selection.  After 15 seconds of not pressing anything, it will time out and return to the moon info screen.

The program takes about 700MB of spiffs (in the format of LittleFS) to store the moon images so after you compile and startup the program to format the spiffs/LittleFS partition, do a data folder upload to get the moon images.  As has been pointed out to me, there is a lot of similar pictures and I could pare it down but for the small space it takes, I am not going to do that for now.  You can if you wish, of course.  That complaint came in about 25 years ago when this was a VB program for PCs.  It was not a big enough concern then, either.  Disk storage is cheap and huge and network speeds are fast.  

If there is any problem with that or any other part of this little toy program, please let me know.  

Mikey the Midnight Coder

![MP Main](https://github.com/user-attachments/assets/5a652f26-739e-4a17-aacf-2e4484025942)
![Phases](https://github.com/user-attachments/assets/beb41764-c65c-48c0-ae8f-854e0718ca39)
![Sun Info](https://github.com/user-attachments/assets/ccb0a1a8-243c-4e80-9cd9-4913d3873078)
![Where's the Moon](https://github.com/user-attachments/assets/a7526efd-c642-43fc-9940-0ddaec0bf534)
![Menu](https://github.com/user-attachments/assets/03ec1185-ab6f-421a-a286-1126676a243b)
