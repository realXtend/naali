This test scene implements avatars. It uses two script files: jsmodules/avatar/avatarapplication.js, which hooks
to connects/disconnects of users and creates/deletes avatars for them, as well as handles the client's camera switching
(ctrl+tab), and jsmodules/avatar/simpleavatar.js, which implements movement & animation of a single avatar.

Compared to the original Avatar example, this scene optimizes serverside CPU usage by using the EC_PhysicsMotor 
component instead of applying movement force in script, does not animate avatars on the server, and does not handle
avatars' physics collisions in the script.

To test, copy default_avatar.xml, fish.mesh & WoodPallet.mesh to your tundra bin/data/assets directory, and avatar.xml
to your bin directory.

Then, run the server and load the scene on it by drag-and-dropping scene.txml to the main window, or by using console
command loadscene(scene.txml)

Next, start one or more clients and connect to the server. Each client should get an avatar that can be controlled
with WASD + arrows + mouse. In addition there is a example how to make addons to the default functionality of 
simpleavatar.js. exampleavataraddon.js adds Q to make a wave gesture and E to toggle sitting on the ground. 
Mouse scroll and +/- zooms in/out.

Only third person camera is currently implemented. Also note that collisions are disabled when flying for now.

The scene AvatarSlideTest.txml is a testbed for observing how avatars behave when sliding along surfaces, and it contains
surfaces with different slopes.