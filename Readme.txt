About

This is an interrupt based keypad library. It is designed and implemented using Object Oriented Design
concepts and program to interface concept.
The interface implementation will be injected to the component that requires it to have dependency inversion
using dependency injection.
The timer component is complete and tested.
The gpio component is under development.
Once GPIO component is complete, then the keypad interface will be implemented

Folder Structure

    components

        The components folder contains different components. The interface folders only contain header file 
        of the interface which is implemented by the implementation folders (Actual components)

    test
        The 'test' folder at the root folder is the test project that uses "unity test framework"
        Each component folder (timer component right now) also contains a test folder which contains
        uniot tests
