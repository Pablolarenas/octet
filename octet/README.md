# Introduction to Programming 2016 - Assignment 1  
# Pablo Larenas
  
--------------------------------------------------------------------------------   

**1. A BRIEF CONTEXT PRIOR THE DEVELOPMENT OF THIS PROJECT**  
As a graphic designer, my foundations in code were minimum at the start of this course. In the first lectures was nearly impossible for me, understand any of the concepts or programming terminology of C++ language. My previous experience with languages was related to web design work with HTML 5 and CSS 3. That way, I was expecting to learn the basic concepts, syntaxes and the logic behind that new language (C++, C#) first  and then ‘create something’ from this foundation, but I was completely wrong…  

**2. APPROACH: DESIGN AS A STARTING POINT**  
To face this assignment (create a new 2d game from invaders example) I started from the place I had some experience: concept and idea of the game, plus some initial mechanics. Defining this in the first place was a useful approach to create a mind map of what kind of things I needed to learn to create the game in a successful way. The benefit of working through a design pathway was that I could organize my workflow in an early stage instead of improvising erratically all the time and get frustrated in the way. 
I briefed an initial game concept in a simplified storyboard on Illustrator (just squares, triangles and circles with descriptions). Then, I presented it to my lecturer and validate immediately the scope and viability of my idea. From that clear concept diagram, I started to work in the code.  
  
**3. THE GAME**   
- ***3.1 Description***
	“Sneak” (called Diamond during the process) is a 2D single player game which you assume the identity of a thief who wants to 		steal a diamond in a mysterious tomb.  
	The camera is on top of the stage, so the player can see the entire room, the diamond and the different obstacles that must sort 	 to reach the treasure, but when the tomb locks, the darkness turns the traps less visible for the player.  

- ***3.2 Gameplay:***
	• The player is represented by a character (Thief) which is used to play the game.
	• The character (Thief) can be moved in the primary up/down/left/right directions.
	• Successfully navigating until to collide with the ‘goal’ (a diamond) the player finishes the level. 
	• The player must find a path to reach the diamond, avoiding the ‘enemies’ (skulls) and the traps in the level.
	• If the Thief collides with the skulls, the player loses, and must start again.
	• If the player passes over a trap the skulls start chasing the Thief until collide with it. 
	• In this situation, the player can try to reach the diamond before the skulls can collide with it.
  
  
**4. MAIN GAMEPLAY PROGRAMMING**  
- **‘in game’ menu.** To create it, I initialized only the menu elements in the ‘app_init’ function, and just left the resource_dict of the game textures and sound files. The level texture’s inits were moved to a “load_game” function, called in the “simulate” function, to that way I can start the program just with the menu objects: Game logo, Thief (just for select options), an ‘About’ button to describe the game and credits and the ‘Start’ button, to load the level. Once, the thief collides with the start, it is moved and resized at the top of the screen to start the game.  
  
- **Random positioning of elements system (diamond, thief and traps)**, using a random system where ‘x’ and ‘y’ axe position where defined by r1 and r2 values. Both of them, a random number between two numbers (r1 = rand() % 19 – 9). 
To avoid undesired colliding between objects (thief and traps or thief and diamond) at the beginning of the game, I defined different random conditions (numeric range) for those different objects. That way, those assets will be loaded randomly but in different areas of the level.  

- **The “load_game” function**, in order to organize my game in two parts (menu + stage) I created a function which initialize the game elements initialization (thief, traps, diamonds, borders, skulls, win/lose text) and translate the menu elements away of the screen at the same time.  The “load game” function is called when the character collides with the ‘start’ button.  

- **Conditional texture switching**, created for thief wen it changes movement direction (using ‘if’ statement), for the skulls when they go to chasing mode (creating a “change_sprite” function in the sprite class, called later in the “chasing” function of the game) and for traps and background when the game starts (creating a “camouflage_traps” function, called in the “simulate” function if the frame’s number equals 60 (if (frames == 60)).  

- **Chasing system for the skulls** based on the position of the thief’s sprite. Using the ‘if’ statement, comparing x and y position of the thief and skulls if (thief_sprite.x < sprites[first_skull_sprite_index+j].x) movement_x = movement_x * -1;.  

- **Shaders.** At the beginning of the project. I started like anyone else to ‘play with the code’, to figure out the functionality of different parts of it. That way, I could modify one of the shaders files (texture_shader_h) , making a kind of gradient adding “‘gl_FragCoord.y/500.0f” at the end of the gl_FragColor’ value in the ‘main’ function, but I noticed that that line modified all the sprites in the game.  

- Finally, with the assistance of Matthew Duddington, I could be able to change the color of the skulls in the chasing, instead changing the sprite. The process basically consisted in adding an optional color parameter to the “texture_shader_h” file, that later will be rendered in the game in certain conditions.  

- **Implementing up/down/left/right directions to the character.** Editing the ‘if’ statements for different button instances. Also, adding them a ‘change_sprite’ functions and calling a different texture depending of the orientation of the movement.  

- **Failed attempt: CSV files**  
About the use of CSV files, I know what are they used for. Actually, I wanted to use a CSV file to define the number of diamonds, traps and skulls, but I couldn’t make the code read it.  
  
  
**5. LEARNINGS**  
The most important thing I learned is to read the code and not get lost in the attempt. It takes me more than few seconds to understand it, but now I can read it and extract the main idea of it (If the line is a function, a variable, a statement or if its related to an specific aspect of the game, is calling a function, a texture, a sound, etc.). So, in general terms, I don’t feel myself as an alphabet anymore.  

Now, I can discriminate between functions and variables, I can deduce that an error could occurred because the variable hasn’t been declared or certain function doesn’t appear in the game because is not simulated. How, where and why to write a function. When to use statements For, if, else and while and what kind of things I can do with them. Also, during the process I learned general concepts as:  

- **Declare the variable first if I want to work with it later.** Something very basic, but not for non-programmers. Understand how the machine works, how the compiler reads the code and what in the logical pathway to “make something”.  
- **Work clean:** use logical and descriptive names for your variables and functions to avoid a mess in your code!  
- **Work with different sprites**, in order to organize and facilitate the “for” loop writing and some functions, I realize that in this simple game, some elements, due to their nature, use in the code, relevance etc, deserved to be located in different arrays. After, this it was much more simpler for me work with the rest of the code.    
- **Concept of Array and Index,** being able to manipulate and interpret them, f.e: change and control the random locations of level objects. 

**6. SPECIAL THANKS**  

- To Pedro Quijada, who taught me the fundamentals of programming, how to think in a programming logic and stayed next to me during the process several times with his good energy and advices that helped me to improve my project.  

- To Nick Veselov, who patiently taught me the concept of array and how to use Visual Studio tools at the beginning of the project.  

- To Matthew Duddington, who kindly help me to understand several aspects of the invaders code and taught how to work with arrays and indexes in a proper way. Also, he explained to me how to add new color parameters to the shader file.  
  
    
    
**7. SOURCE FILES**  
  
**7.1 Gaphic Elements**  
All textures were designed and drawn by me, using Adobe Photoshop and Adobe Illustrator, both licenced under my personal Creative Cloud Account.  

**7.2 Sounds**  

Lose  
Composed sound in Adobe Premiere. licenced under my personal Creative Cloud Account  

Door  
Composed sound in Adobe Premiere. licenced under my personal Creative Cloud Account  
  
The following music and sound are extracted from other computer games just as a reference:  

Win  
Extracted and modified music from The Legend of Zelda: Link’s Awakening.  
“you-found-an-item.mp3” from http://downloads.khinsider.com/game-soundtracks/album/link-s-awakening-dx  

Dungeon  
Extracted and modified music from The Legend of Zelda: Link’s Awakening.  
“mini-boss-battle.mp3” from http://downloads.khinsider.com/game-soundtracks/album/link-s-awakening-dx  

Chase  
Extracted and modified music from The Legend of Zelda: Link’s Awakening.  
“mini-boss-battle.mp3” from http://downloads.khinsider.com/game-soundtracks/album/link-s-awakening-dx  

  
