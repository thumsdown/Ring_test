# Ring_test
This is solution to Ring question

I have used state machine to transit from one state to another. The approach that I have followed is controlling the state of my machine runtime. I have taken care that I dont have to maintain a static table to map events and transition.
Advantage with this approach is  This allows us to forward-reference states as we define them. 
This is still a work in the progress.
