# 6-shell-2

    In this version of my shell, I am able to implement a few critical structures to my code. I am able to handle three important concepts: signals, foreground and background processes. I set up my shell to have ignored three basic signals of suspension and termination. Only SIGQUIT is set to its default handler. All my child processes are abled to recieve signals from their default handlers. 

    In terms of foreground processes and background processes, I handle two waitpids. One waitpid for foreground processes after exec where I wait for that particular foregorund to finish its execution unless a signal is sent. The other waitpid is used for background processes in my reaping method where I go through all my child processes still in existance and check to see if they have been terminated or suspended.

    I then implement the & concept which specifies a background process or not. If no ampersand is found, then add to job list and give temrinal control to the shell. I parse for this &.

    There is also the fg and bg built-in commnads, where I give terminal control to the process id for foreground control or if it is a background process I give it to the shell. In both cases, I use the kill command to send the SIGCONT signal to the processes to be RUNNING. For foregoround methods that are not continued, I must make sure to wait for the process to eiter eixt or be terminated or suspended by a signal.


