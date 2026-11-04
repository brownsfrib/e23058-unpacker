#!/bin/bash
git config core.sshCommand "ssh -i ~/.ssh/id_ed25519_brownsfrib -o IdentitiesOnly=yes"
git push -u origin main
