acPostProcForPut{
  ON($objPath like "/$rodsZoneClient/home/$userNameClient/event/*"){
    msiWriteRodsLog("LOGGING: object", *Status);
    msiWriteRodsLog("$objPath triggered event hook", *Status);
    msiAddKeyVal(*Keyval,"ibridges:acltrigger","acPostProcForPut");
    msiGetObjType($objPath, *objType);
    msiAssociateKeyValuePairsToObj(*Keyval,$objPath,*objType);
    msiWriteRodsLog("default, own, datastewards", *Status);
    msiSetACL("default", "own", "datastewards", $objPath);
    msiWriteRodsLog("default, read, $userNameClient", *Status);
    msiSetACL("default", "read", $userNameClient, $objPath);
    msiWriteRodsLog("LOGGING END", *Status);
  }
}

acPostProcForPut{
  ON($objPath like "*/santa.txt"){
    msiWriteRodsLog("$objPath triggered event hook", *Status);
    msiGetObjType($objPath, *objType);
    msiAddKeyVal(*Keyval,"Santa says","Merry Christmas!");
    msiAssociateKeyValuePairsToObj(*Keyval,$objPath,*objType);
  }
}

acPostProcForPut { }

acPostProcForCollCreate{
  ON($collName like "/$rodsZoneClient/home/$userNameClient/event/*"){
    msiWriteRodsLog("LOGGING: Collection", *Status);
    msiWriteRodsLog("$collName triggered event hook", *Status);
    msiAddKeyVal(*Keyval,"TRIGGER","acPostProcForCollCreate");
    msiAssociateKeyValuePairsToObj(*Keyval,$collName,"-C");
    msiWriteRodsLog("LOGGING END", *Status);
  }
}

acPostProcForCollCreate { }
