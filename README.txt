DESCRIPTION
------------------------------
This is a plugin that makes it possible to connect a special sound to the
arrival of a message from specific contacts. That has been a feature of
the Mirabilis’ client almost from the beginning but there has been no way
to get this in Miranda as far as I know.

With this you could have a list with sounds like following:

 * Contact List
    John -> train.wav
    Mary -> flower.wav
    Igor -> default sound

You change the sound by opening a contact's user info and selecting the
event sound tab.

INSTALLATION
-----------------------------
Just drop the PersonalizedSoundPlugin.dll file into Miranda's plugin 
directory.

KNOWN ISSUES
---------------------------
There is no way to turn off the default sound selectively so the OS will
be ordered to play both the default sound and the special sound. This
doesn't seem to be such a big problem after all. I guess the second "play"
overrides the default sound so quickly that only the correct sound is
heard. I also guess it could be the other way around if the modules are
called in a different order or both sounds being played at the same time
if the OS/sound card allows that. This is something that will have to be
changed in the Miranda platform (msgs.c)


KNOWN LIMITATIONS
---------------------------
Regardless of the type of message a contact sends, the same sound is
played. In the future it could be possible to connect one special sound
to the message arrival, one sound to a URL arriving and the default sound
for a file arriving.

TESTING NEEDED
---------------------------
I have only used ICQ so the plugin hasn't been tested with other protocols.

It also hasn't been tested in any other language than English.

FEEDBACK
---------------------------
Feedback should be sent to me at bratell@lysator.liu.se.

The plugin's homepage is 
http://www.lysator.liu.se/~bratell/PersonalizedSoundPlugin