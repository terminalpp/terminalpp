# Creates a self-signed certificate for the msix package
#
# From https://docs.microsoft.com/en-us/windows/msix/package/create-certificate-package-signing
#
# Usage: create-certificate ps1 PASSWORD


# create the certificate and make it valid for 100 years
$cert = New-SelfSignedCertificate -Type Custom -Subject "CN=333F5A3D-D64B-4F2D-A47C-2EA258D1B12E" -KeyUsage DigitalSignature -FriendlyName "Terminalpp - msix" -CertStoreLocation "Cert:\CurrentUser\My" -TextExtension @("2.5.29.37={text}1.3.6.1.5.5.7.3.3", "2.5.29.19={text}") -NotAfter (Get-Date).AddMonths(12*100)

# create the pfx certificate with password and zip it as well
$password = ConvertTo-SecureString -String $args[0] -Force -AsPlainText 
Export-PfxCertificate -cert $cert -FilePath terminalpp-cert.pfx -Password $password
Compress-Archive -Path terminalpp-cert.pfx -DestinationPath terminalpp-cert.zip

# now create a base64 encoded version of the certificate that can be used in GH actions
$cert_pfx = get-content "terminalpp-cert.pfx" -Encoding Byte
$cert_base64 = [System.Convert]::ToBase64String($cert_pfx)
Out-File -FilePath "terminalpp-cert.pfx.base64.txt" -InputObject $cert_base64 -Encoding ASCII

# export the certificate w/o the private key
Export-Certificate -Cert $cert -FilePath terminalpp.cer





