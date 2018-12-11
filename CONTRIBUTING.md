# Contributing to OPRECOMP

We provide a quick introduction for the following topics:

1. Start using Git for OPRECOMP
2. Your private development
3. Getting the latest changes from the OPRECOMP main repository 
4. Putting your work to the OPRECOMP main repository
5. Sharing information over Git with partners
6. Tipps and tricks

Comments are always welcome. 

# 1. Start using Git for OPRECOMP
First, you require an account. If you not yet have one, please request on by sending an email to [Fabian Schuiki][email].
Second, follow the instruction on the activation email we send back to you. Now, you should be able to access gitlab.

Third, a few words about the policy we have to keep OPRECOMPS git tidy. We use the following main repository `oprecomp/oprecomp` that holds the latest version of contributions of the project. However, nobody except we have WRITE access to that project. Only reviewed, 
unit-checked code and finalized part of software are supposed to be in that repository. If you are are at a stage of contributing your work to the oprecomp repository, please follow Section 4. For sharing code under development with our partners, see Section 5.

For your development work, we provide you an own repository. In order to achive that, you need to follow two simple steps:
1. Fork the project oprecomp/oprecomp and create a repository `<YOUR NAME>/oprecomp`:
   - In gitlab go to the [upstream] `oprecomp/oprecomp` repository
   - click on "fork"
   - fork the project for the user you just created
   - you should now see a repository `<YOUR NAME>/oprecomp` 
2. Clone the repository to your device:
   - Type the following into your terminal (you can copy & past your URL directly from gitlab)

Use the following command:

    # clone your git repository
    git clone git@iis-git.ee.ethz.ch:<YOUR NAME>/oprecomp.git

Do you encounter access problems? You are not required to enter a password, instead you can simply add the public key of your device to gitlab.  
Please follow the instructions on `>> top right drow down >> settings >> SSH key` or visit https://iis-git.ee.ethz.ch/profile/keys to add the SSH key of your machine to gitlab. Please note, that you are required to repeat that step for each machine if you plan to use that repository on multiple devices.

# 2. Your private development
Work with best-known practices on your repository. If you need help you can visit the following ressources:
- https://guides.github.com/activities/hello-world/
- https://git-scm.com/book/en/v2/Git-Branching-Basic-Branching-and-Merging

Basically, you need the following to add changes:


    git add <FILENAME>
    git commit -m "Message that describes your changes"

To push your changes to our server:


    git push
    
And to get your data from the server:


    git pull
    
    
Monitor your work is achived with:


    git status

# 3. Getting the latest changes from the OPRECOMP main repository

First, add the main git repository:

    # Adds a new remote host under the name "upstream"
    git remote add upstream git@iis-git.ee.ethz.ch:oprecomp/oprecomp.git
    # List what you just added to your settings
    git remote -v
    # Change or further information
    git remote -help
    
Second, you can download the latest data from oprecomp by the following command:


    git fetch --all
    
Next, rewind your local changes and jointly apply global and your local changes:


    git rebase
    
In case of merging conflicst, resolve them one-by-one. 



# 4. Putting your work to the OPRECOMP main repository

The [upstream] repository is hosted by ETH Zurich. To start hacking away, please create your own fork of this repository on [GitLab]. To do this, click the "*Fork*" button on the repository's website. You may also want to grant the `oprecomp` group *Reporter* access to your fork on the *Members* settings panel. To contribute your work back, either open a merge request on [GitLab], send us an [email] with the patch, or send us an [email] with the location of your clone where we can merge from.

[upstream]: https://iis-git.ee.ethz.ch/oprecomp/oprecomp
[GitLab]: https://iis-git.ee.ethz.ch/
[email]: mailto:fschuiki@iis.ee.ethz.ch


## 4.1 Using Merge Requests

If you have your own fork of the [upstream] repository on [GitLab], the easiest way to get changes merged into upstream is by creating a merge request as follows:

1. In your repository, create a new branch that contains whatever you want to be merged, e.g. `git branch my-code-fix`. This is especially important if you have your code in your `master` branch.

2. Go to your project on [GitLab], click the plus icon, and choose "*New merge request*". Pick the branch you created above, e.g. `my-code-fix` as the source, and choose `oprecomp/oprecomp master` as the target.

3. Fill in the details in the following form, and you're done. Make sure that you do not push new commits to this branch, as these will be part of the merge request. Either use [feature branches], or if you're working on your `master` branch, at least create a separate branch for the merge request.

[feature branches]: https://www.atlassian.com/git/tutorials/comparing-workflows#feature-branch-workflow


## 4.2 Using a URL

If you have your own repository accessible somewhere, either via SSH or via HTTP, simply send an email to [Fabian Schuiki][email] with the URL and the commit-ish (branch, commit, or tag) you would like to have merged.


## 4.3 Using Patches

If you choose to use Git in a more bare-metal fashion, you can also submit your patches via email. Use [git-format-patch] to create a patch of your work. For example:

    # create a patch with all commits not in master
    git format-patch master --stdout > fix_stuff.patch

    # create a patch with the last 3 commits
    git format-patch HEAD~3 --stdout > fix_stuff.patch

Please send your patch file to [Fabian Schuiki][email].

[git-format-patch]: https://git-scm.com/docs/git-format-patch

# 5. Sharing information over Git with partners

The best practice to share information between partners work similar as to share information with the [upstream] repository. We suppose to have to partners A and B that want to share information. We assume that A and B have their own forks of the oprecomp repository, namely A/opercomp and B/oprecomp.
Both partners are required to add the repository of the other part to their git settings.

Steps to share information:
1. A and B required to set the correct visibility of their project
2. Inform partners that they are aware of change
3. Work with the new access

# 5.1 Project settings
On GitLab go to the `Member` ribbon and check at `Existing members and groups` which users alredy have access to your work.
Next, navigate to the `Select members to invite` ribbon and add single users you want to share your work with.

You can use the `Share with group` ribbion to share your fork with all internal members of the oprecomp group. Give reporting access to all people belonging to that group. 

Select `Reporter`, which allows your partner to get READ access but no write access to your project such that it stays under your full control. Full information
about access permissions can be found here: [https://iis-git.ee.ethz.ch/help/user/permissions].

# 5.2 Inform partner
Next, inform your partner about what you are sharing. Note, that this way is used to give access to bleading-edge development, that still might
contain bugs, errors or work in progress. Reporters can get ideas and become aware of work in progress. However, they should never base their own work on
code obtained that way! That might cause unnecessary (merge) problems in future work and discrepancy between partners. The official way to base work on each other
is via a merge request into the [upstream] `oprecomp/oprecomp` repository.


# 5.3 Access the partners work
Either you have a quick look at the files via GitLab or you can directly integrate the partners on your local working machine:

    # command for A that want to keep track of the working state of B.
    git remote add B git@iis-git.ee.ethz.ch:B/oprecomp.git
    # and vice versa, the command for B to keep track of A's work
    git remote add A git@iis-git.ee.ethz.ch:A/oprecomp.git


The following command allows to control the remote repository by listing the current registered remote repository.

	git remote -v

Next, we can issues a download from the server to get the full state of the partner's work.

	git fetch --all

With

    branch -av

you see now the state of the repository of your partner and you can easily switch forth and back.


# 6. Tipps and tricks

If you are unsure about a situation, or you want to try a new command, do the following:

First, make a branch as backup of the current state:


    git branch bkup

Next, work on your current branch (master or some other branch) and do the changes and commands you want to do.
If you succeed, and you do no require the backup branch any more you can delete it:


    git branch -d bkup

However, if you are encountering problems and you have messed things up, you can start over with the following:


    git reset —hard bkup

The following are some useful commands to monitor the state of your git:


    # Show the branches you have in your local git-repository
    git branch
    # Show the branches you have registered with more detailed information
    git branch -av
    # Show the history of your repo
    git log --oneline --decorate --graph





