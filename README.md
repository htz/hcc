docker run --rm -it --cap-add=SYS_PTRACE --security-opt="seccomp=unconfined" -w /mnt -v `pwd`:/mnt dqneo/ubuntu-build-essential:go bash


brew cask cleanup --outdated
brew update;brew upgrade;brew cleanup; brew doctor
