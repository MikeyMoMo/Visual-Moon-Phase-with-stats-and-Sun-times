# Visual-Moon-Phase-with-stats-and-Sun-times
This is mostly a visualization of the moon phase and a tutorial on invisible-color sprite usage.

See the pictures, below, for a better idea what this does.  It was created as a simple little moon phase display.  It then got a moving starfield behind the moon.  It then got a little 2-button menu (not using a library).  The moon is inserted with an invisible color to cut off the four corners of the jpg then that is put onto another sprite that has a randomly created starfield on it. There are multiple sizes and color (pale blue, pale red, white and grey) of stars to make it look somewhat realistic and they scroll to the left as new stars come on from the right.

I did not do rotation because it very much depends on where you are and when you are as to what rotation you will see.  I may put it in some day.  The code to do the rotation is there but commented.  If you want to try it, go ahead.  The program gets the time of day and uses DST rules for any place on earth.  The moon can be turned on and off.  I just did that to see the entire starfield.  It also has a 3-item menu that will show you the next four major phases of the moon and sunrise and set times for your location and time.  You can turn the moon on and off with that menu, too.  Let me see someone do that with the real moon!!

The program takes about 700MB of spiffs (in the format of LittleFS) to store the moon images so after you compile and startup the program to format the spiffs/LittleFS partition, do a data folder upload to get the moon images.  As has been pointed out to me, there is a lot of similar pictures and I could pare it down but for the small space it takes, I am not going to do that.  You can if you wish, of course.  

If there is any problem with that or any other part of this little toy program, please let me know.  

Mikey the Midnight Coder

![MP Main](https://github.com/user-attachments/assets/5a652f26-739e-4a17-aacf-2e4484025942)
![Phases](https://github.com/user-attachments/assets/beb41764-c65c-48c0-ae8f-854e0718ca39)
![Sun Info](https://github.com/user-attachments/assets/ccb0a1a8-243c-4e80-9cd9-4913d3873078)
![Where's the Moon](https://github.com/user-attachments/assets/a7526efd-c642-43fc-9940-0ddaec0bf534)
![Menu](https://github.com/user-attachments/assets/03ec1185-ab6f-421a-a286-1126676a243b)
