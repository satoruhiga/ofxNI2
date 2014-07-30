OSX:

Add to Run Script

	cp -R ../../../addons/ofxNI2/libs/lib/osx/ "$TARGET_BUILD_DIR/$PRODUCT_NAME.app/Contents/MacOS/";


Copy data/NiTE2 to $TARGET_BUILD_DIR/data/NiTE2


Linux:

Copy data/NiTE2 to $TARGET_BUILD_DIR/data/NiTE2
Copy libs/lib/linux64/NiTE.ini to $TARGET_BUILD_DIR/NiTE.ini
Copy libs/lib/linux64/* to $TARGET_BUILD_DIR/libs/

Note: copy libs BEFORE compiling. 
