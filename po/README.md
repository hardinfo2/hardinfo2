Translators
===========

Before starting translation of the words/texts, that we have in the program, the po files
needs to be updated
 - cd po            (Always be in the po folder)
 - ./updatepo.sh    (Updates po from source code and shows status)

When done the .po files are ready to be changed, thanx for helping with translation.


Status
------
This is an example of status of the different languages including your own after
editing - use the ./updatepo.sh

hardinfo2.pot now has 1329 strings (no change), with 63 c-format strings
(as of 0bf1cd35-dirty 0bf1cd35956586e01807ac6e8829f845ef56f097)
- [x] da.po : (0 / 1329 remain untranslated, needs work/fuzzy: 0)
- [ ] de.po : (856 / 1329 remain untranslated, needs work/fuzzy: 2)
- [ ] es.po : (454 / 1329 remain untranslated, needs work/fuzzy: 18)
- [ ] fr.po : (871 / 1329 remain untranslated, needs work/fuzzy: 0)
- [ ] hu.po : (1165 / 1329 remain untranslated, needs work/fuzzy: 0)
- [ ] ko.po : (228 / 1329 remain untranslated, needs work/fuzzy: 0)
- [ ] pt_BR.po : (23 / 1329 remain untranslated, needs work/fuzzy: 0)
- [ ] pt.po : (217 / 1329 remain untranslated, needs work/fuzzy: 0)
- [x] ru.po : (0 / 1329 remain untranslated, needs work/fuzzy: 0)
- [ ] tr.po : (18 / 1329 remain untranslated, needs work/fuzzy: 0)
- [ ] zh_CN.po : (933 / 1329 remain untranslated, needs work/fuzzy: 0)

Needs work/fuzzy typically has to do with spaces/periods at begin/end of translation.


Editing
-------
Use the poedit program installed by:
 - apt install poedit  (Debian flavours)
 - yum install poedit  (Redhat flavours)

Start the program:
 - poedit ./xx.po   (xx=LANGUAGE-2LETTER-SMALL-CAPS)

NOTE: In poedit please only do translation and use find, validate and save.


New Language
------------
 - cp NEW xx.po  (xx=LANGUAGE-2LETTER-SMALL-CAPS)
 - emacs ./xx.po    (edit the empty po and change all CHANGE-* accordingly)
 - ./updatepo.sh    (updates the po with all the words/text we have in the program and shows status)
 - ready to edit - see above


Backup and Commit to GitHub
---------------------------
 - git add xx.po    (If you have made a new language translation)
 - git commit xx.po (Just commit your changed translation file to local repo)
 - git push         (Push it to github)

It is better to make a git push too much than loose all your great work.
Update gettext translation files.

