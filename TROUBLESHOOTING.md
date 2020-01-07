A. You get a message about a9lh or shadowNAND. 
- Handling for arm9loaderhax and its various implementations is still experimental. While simply pressing A on this prompt is significantly safer than the old process of renaming key.bin, you should still try to upgrade to b9s without using b9stool first. If you do decide to press A to continue on this prompt, one of two things should happen:<br>
- You will reboot and see a modded version of safeb9sinstaller for a few seconds, then you will land in luma3ds config. This means you had an arm9loaderhax implementation installed, it was successfully removed, and you are on boot9strap now.<br>
OR<br>
- You will reboot and simply see the home menu again. This means you didn't have arm9loaderhax installed. If this happens, you should run b9stool again only once, and the check for a9lh will be bypassed. After running b9stool the second time, you should see luma3ds config.<br>
Visit this Discord channel if you have any questions: https://discord.gg/C29hYvh <br>

B. You select "Install boot9strap", reset the system, but the system just boots up normal.<br>
- Go to System Settings > Other Settings > Page 4 > System Update. Make sure to actually attempt to update with wifi on.<br>
It should say "This system is up to date". Press ok and let the system fully reboot to home menu on its own. <br>
Boot b9sTool again.<br>
Choose "Install boot9strap", and hopefully you will see luma3ds config on next reboot.<br>
