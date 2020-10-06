# Windows Store Submission

## Automatic Binary Update

For more details see the following links:

- https://docs.microsoft.com/en-us/windows/uwp/monetize/manage-app-submissions
- https://docs.microsoft.com/en-us/windows/uwp/monetize/update-an-app-submission

There is a powershell module that should help with this:

- https://github.com/microsoft/StoreBroker

> The store broker has a simple-ish and concise documentation on how to create the AD for the project and how to do create the application user that will be used to do the submissions. 

The store api documentation is really extensive, but in the end all that needs to be done is clone the submission, do the existing package as pending removal and add new package of same name (no need to fill in version). Then the `msix` must be put in a `zip` archive and uploaded. 

That's it. 

The one thing to mind is that until certified, the partner center does not show the updates made by the submission properly. 

> There is no need to deal with the jsons and xmls the store broker discusses in setup because I am only even updating the binary through the CI. 

> Also, store broker can create packages for me, but not of `msix` in its current version. Not that I need them now. 