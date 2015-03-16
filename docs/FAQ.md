## Where's the Last.fm FAQ?

  * http://www.last.fm/help/faq


## What does "The Webservices API key supplied does not have permission to stream / create new radio trials" mean?

This means your 30 track free trial has expired.

## What does "Radio station not supported with this country/client combination" mean?

This means you must be a subscriber to listen to Last.fm radio outside the US, UK and Germany (other than free trials). See also the next question.

## I'm not a subscriber and I can't play radio any more, why not?

Mobbler versions 0.10.x and 2.10.x uses the official Last.fm radio API which only allows paying subscribers to stream the radio. Non-subscribers with free accounts are allowed a 30 track free trial from Last.fm. Only paying subscribers can play the radio. Free user accounts can't play the radio. Subscribers will see a [prestigious black icon](http://www.last.fm/subscribe) at the top left when online. If you have an S60 phone (i.e. pre-N8/Symbian^3) an older version may still stream radio until Last.fm disable it.

See also:
  * http://www.last.fm/group/Last.fm+Web+Services/forum/21604/_/517212

## I just updated but when I run Mobbler it's still the same one. What's up?

Due to an Ovi Store signing limitation, Mobbler will install as a new application. Please make sure you are using version 2.x(x) not 1.x(x).

Symbian-Signed versions have major version 1 or 2 and a solid red logo.
Self-signed beta versions have major version 0 and the crazy paving version of the logo.

If you did the update you should now have both versions of Mobbler installed. It is recommended that you uninstall the old version of Mobbler (after submitting any queued scrobbles) and continue to use the new version.

We hope to only release Symbian-Signed versions from now on and use self-signed
versions for beta testing.

## I get an "Update error" when installing Mobbler 0.8.x or later.  How can I fix it?

Please uninstall "mobbler helper" from your phone, you don't need it anymore. You can remove it from the Application Manager (App. mgr.).


## When will scrobbling be supported for S60 5th edition phones like the Nokia 5800 and N97 and 5530 XpressMusic?

[Now!](http://code.google.com/p/mobbler/downloads/list?can=2&q=label%3AFeatured+sisx&colspec=Filename+Summary+Uploaded+Size+DownloadCount)

## How about scrobbling from Spotify for Symbian?

Mobbler cannot scrobble from Spotify for Symbian because there is no way to observe
what it is doing. It is something that Spotify will have to add themselves. They have said that it's something they are looking into.

See also:
  * http://getsatisfaction.com/spotify/topics/last_fm_scrobbling_on_mobile_app


## Why do I get a certificate error when installing Mobbler?

It probably means that your phone does not accept _self signed_ applications. You will need to go to the application manager on your phone and change the software installation setting from _signed only_ to _all_.

## How can I make Mobbler start automatically when I turn on the phone?

There are a number of third party autostarters you can use, for example PowerBoot.

See also:
  * [Issue 27](https://code.google.com/p/mobbler/issues/detail?id=27)

## My settings are set to scrobble at 50% but why does it wait for the track to finish before it is scrobbled (or queued)?

According to [Last.fm's rules](http://www.last.fm/api/submissions#subs), the decision whether to scrobble a track can only take after the track has finished playing. For example:

  * If the track stops just before 50%, it won't be scrobbled (or queued).
  * If the track stops just after 50%, it will be scrobbled (or queued).
  * If the track stops at the end (at 100%), it will be scrobbled (or queued).

## I can stream radio over wifi, but not over a data connection. How can I fix it?

Many people seem to have problems with access points on their phones. Check if the access point you are using is using a proxy. Some people have said that access points with proxies are not able to stream mp3s.

This setting is in the advanced settings of your access point. If it uses a proxy, change it to none and try again.


## With 3UK or 3 Ireland, I keep getting 403 errors whilst streaming radio. How can I fix it?

Duplicate your **3 access point** and change **three.co.uk** to **3internet**.

See also:
  * http://nokiaaddict.com/2009/05/20/mobbler-v0-0-45-on-e71-via-3uk
  * http://gadgeteer.org.uk/2009/05/22/still-having-mobbler-problems-on-3uk


## With Vodafone (Netherlands or Spain), I keep getting 403 errors whilst streaming radio. How can I fix it?

A Vodafone Netherlands and a Vodafone Spain user have reported this works: Remove all access points with proxies. It is not enough to remove only the proxy for the access point actually being used for Mobbler. Vodafone puts a lot of access points with proxies on the phone. Remove them all and leave only one accesspoint for wifi and one for UMTS. Probably Vodafone doesn't need proxies. To remove the proxy you have to select advanced settings at the access points and clear the fields for  "Proxy server address" and "Proxy port".


## With O2, the song buffers and plays perfectly for 43 seconds then stops every time. How can I fix it?

Change your default internet connection to 02 Mobile Web from O2 Active. For example with a Nokia 6220 Classic: Menu > Phone settings > Connection > Destinations > Internet.

Or if that doesn't work, follow the step-by-step instructions in [here](http://code.google.com/p/mobbler/issues/detail?id=406#c5).

See also:
  * http://code.google.com/p/mobbler/issues/detail?id=406#c2
  * http://code.google.com/p/mobbler/issues/detail?id=406#c5


## With Giffgaff, the song buffers and plays perfectly for 43 seconds then stops every time. How can I fix it?

Giffgaff is a mobile virtual network operator using the O2 network. Removing the proxy should help, see this discussion for more tips:

  * http://community.giffgaff.com/t5/Help-Ask-the-community-got-stuck/Mobbler-always-stops-playing-last-fm-song-at-exactly-43-seconds/m-p/119551


## When playing radio on AT&T, it connects, fetches playlist and album art, but just sits at 00:00 without playing. How can I fix it?

Some people have said in [issue 246](https://code.google.com/p/mobbler/issues/detail?id=246) that it will work on AT&T when selecting either "AT&T Internet" or "Operator - Proxyless" as the access point. These ones don't have proxy settings defined in the advanced settings.

See also:
  * [Issue 246](https://code.google.com/p/mobbler/issues/detail?id=246)

## When playing radio on Rogers in Canada, it connects, fetches playlist and album art, but just sits at 00:00 without playing. How can I fix it?

We've heard that it will work on Rogers if you (for example with an E71) go to Tools > Access points > Rogers Internet > Advanced options, you'll see a proxy had been set up. Delete the proxy address (looked like a mac address, not IP) and the radio will now play. (It might be a good idea to make a backup copy of the AP first.)

See also:
  * http://code.google.com/p/mobbler/issues/detail?id=246#c21

## With Orange France, after Connecting and Handshaking I get a "500 Internal Server Error" and cannot connect. How can I fix it?

As is often the case, don't use proxy server. For example on a Nokia 5800, go to Settings > Connectivity > Destinations > Internet > Orange World access point > Options, Advanced settings > and delete proxy server address (&port num=0). It's a good idea to keep the existing access point and create a new one (e.g. Orange World 2) with a copy of original settings but without the proxy settings, and then switch between priorities (Orange World & Orange World 2).

See also:
  * [Issue 189](https://code.google.com/p/mobbler/issues/detail?id=189)

## Why won't my Sony Ericsson phone scrobble?

Sony Ericsson phones like the Satio and Vivaz have a different music player that doesn't implement support for the music player observer API that Mobbler uses. We got in touch with Sony Ericsson, but unfortunately their management don't want to make public their API to scrobble their music player.