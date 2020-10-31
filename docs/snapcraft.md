# Snapcraft Releases

First, export the login into a file:

    snapcraft login
    snapcraft export-login --snaps "terminalpp,ropen" --channels "edge,stable" --acls "package_upload,package_access,package_push,package_update,package_release,package_manage" snapcraft-credentials 

Then `snapcraft-credentials` must be stored in a github secret (`SNAPCRAFT`) and can be used for the releases.

> Note that the documentation says the exported login is only valid for a year so this needs to be re-executed every now & then. 


'package_access', 'package_manage', 'package_push', 'package_register', 'package_release', 'package_update'