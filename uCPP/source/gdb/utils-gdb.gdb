# 'server' keyword disables confirmation dialog when re-loading/re-defining
# 'reset' restores the registers to the stack values at breakpoint to allow the movement commands to work correctly
server define hook-continue
reset
end

server define hook-next
reset
end

server define hook-nexti
reset
end

server define hook-step
reset
end

server define hook-stepi
reset
end

server define hook-finish
reset
end

server define hook-advance
reset
end

server define hook-jump
reset
end

server define hook-signal
reset
end

server define hook-until
reset
end

server define hook-reverse-next
reset
end

server define hook-reverse-step
reset
end

server define hook-reverse-stepi
reset
end

server define hook-reverse-continue
reset
end

server define hook-reverse-finish
reset
end

server define hook-run
reset
end

server define hook-thread
reset
end

alias -a clusters = info clusters
alias -a vprocessors = info vprocessors
