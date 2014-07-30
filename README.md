OSX:

Add to Run Script

	cp -R ../../../addons/ofxNI2/libs/lib/osx/ "$TARGET_BUILD_DIR/$PRODUCT_NAME.app/Contents/MacOS/";
    install_name_tool -change libOpenNI2.dylib @executable_path/libOpenNI2.dylib "$TARGET_BUILD_DIR/$PRODUCT_NAME.app/Contents/MacOS/$PRODUCT_NAME";
    install_name_tool -change libNiTE2.dylib @executable_path/libOpenNI2.dylib "$TARGET_BUILD_DIR/$PRODUCT_NAME.app/Contents/MacOS/$PRODUCT_NAME";
    install_name_tool -change libFreenectDriver.dylib @executable_path/Drivers/libFreenectDriver.dylib "$TARGET_BUILD_DIR/$PRODUCT_NAME.app/Contents/MacOS/$PRODUCT_NAME";
    install_name_tool -change libOniFile.dylib @executable_path/Drivers/libOniFile.dylib "$TARGET_BUILD_DIR/$PRODUCT_NAME.app/Contents/MacOS/$PRODUCT_NAME";
    install_name_tool -change libPS1080.dylib @executable_path/Drivers/libPS1080.dylib "$TARGET_BUILD_DIR/$PRODUCT_NAME.app/Contents/MacOS/$PRODUCT_NAME";
    install_name_tool -change libPSLink.dylib @executable_path/Drivers/libPSLink.dylib "$TARGET_BUILD_DIR/$PRODUCT_NAME.app/Contents/MacOS/$PRODUCT_NAME";




Copy data/NiTE2 to $TARGET_BUILD_DIR/data/NiTE2


Linux:

Copy data/NiTE2 to $TARGET_BUILD_DIR/data/NiTE2
Copy libs/lib/linux64/NiTE.ini to $TARGET_BUILD_DIR/NiTE.ini
Copy libs/lib/linux64/* to $TARGET_BUILD_DIR/libs/

Note: copy libs BEFORE compiling. 
