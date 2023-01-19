# Contributing Guidelines

## To Fork or Not To Fork?

When you read this, note that when I talk about forking, I mean it in the sense of starting a spin-off project, not the "technical step" of creating a fork on GitHub. I accept that the latter is required to be able to contribute to the main project.

My hope when I released this work as open source was that people would (a) add cool features that I never thought of, and (b) improve the implementation of things I did think of.  My hope all along is that important and useful features get added to the MAIN branch as soon as possible rather than having 10 forks with support for 10 different LED types.

I realize it's easier to fork a project than it is to properly extend the classes or even the #defines.  But by doing the work in the main tree, everyone reaps the benefits of every other fix and improvement.  I invite you to contribute your important work to the MAIN tree (i.e. the [PlummersSoftwareLLC/NightDriverStrip](https://github.com/PlummersSoftwareLLC/NightDriverStrip) repository) where everyone can benefit from it!

If you're doing something truly obscure, like adding support for an LED type that almost no one else would need, I can see a fork being reasonable.  But if others could in general benefit from what you're doing, please do it in such a way that it goes back into the main tree for everyone's benefit!

_(This is an exact copy of the equally-named section of [README.md](README.md))_

## Contributing, and the BlinkenPerBit Metric

Rather than produce a complex set of guidelines, here's what I hope open-source collaboration will bring to the project: that folks will add important features and fix defects and shortcomings in the code.  When they're adding features, they'll do it in a way consistent with the way things are done in the existing code.  They resist the urge to rearchitect and rewrite everything in their own image and instead put their efforts towards maximizing functional improvement while reducing source code thrash and change.

Let's consider the inconsistent naming, which should be fixed.  Some is camelCase, some is pszHungarian, and so on, depending on the source. I'd prefer it were all updated to a single standard TBD.  Until the TBD is determined, I lean towards [the Win32 standard](https://docs.microsoft.com/en-us/windows/win32/stg/coding-style-conventions?redirectedfrom=MSDN).  

When working in a function, work in the style of the function.  When working on a class, work in the style of the class.  When working on a file, work in the style of the file.  If those are inconsistent, do whatever minimizes changes.  Stylistic changes should only be introduced after discussion in the group, and generally should entain owning that style change across the entire project.

Next, let's consider `#define`s to control the build.  There may be better and more elegant ways of doing things. There could be entire configuration platforms.  But I'd prefer to keep it simple.  And I define simplest to be "the least that an experienced C++ programmer needs to learn before being constructive with the code in question".  I don't want to learn a new class library if I can avoid it!

A lifetime of coding has taught me to err on the side of simplicity, so please don't introduce variadic template constructs unless they demonstrably shrink the source code.  Anything that grows the complexity AND length of the code should be suspect.

Add whatever you want and/or need to make your LED dreams come true.  Fix my blunders.  Fill in the obvious gaps in my knowledge.  Whatever has the most blinken for the fewest bits get my vote.  You only get so much additional cool blinken for every byte of code and program.  That return is measured in BlinkenPerBit, the amount of blinking awesomeness the code adds divided by the impact on the source (and binary).
