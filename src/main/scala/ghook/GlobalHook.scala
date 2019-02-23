package ghook

import scala.collection.mutable.Buffer

object GlobalHook {
  sealed trait HookType
  case object KeyboardHook extends HookType
  case object MouseHook extends HookType
}


/* TODO: Support multiple active hooks */
class GlobalHook(
    pressCallback: () => Unit,
    releaseCallback: () => Unit,
  ) {
  //Load JNI DLL -- does this work across threads?
  LoadNativeLibraries()

  var status: Option[GlobalHook.HookType] = None
  var keyboardHookCode: Short = 0
  var mouseHookCode: Short = 0

  /* Returns input code struct, (keyboard, 0xwhatever) or (mouse, 0xwhatever) */
  @native def getInputKeyCode(): Short

  @native def registerKeyboardHook(): Boolean
  @native def deregisterKeyboardHook(): Boolean


  @native def testFunction(): Array[Short]

  /* Easier to call anonymous functions through JNI this way */
  def changeKeyHook(code: Short): Unit = {
    println(s"Changing Keyboard hook to $code")
    status = Some(GlobalHook.KeyboardHook)
    keyboardHookCode = code
  }

  def changeMouseHook(code: Short): Unit = {
    println(s"Changing Mouse hook to $code")
    status = Some(GlobalHook.MouseHook)
    mouseHookCode = code
  }

  def pressFunc(): Unit = {
    pressCallback()
  }
  def releaseFunc(): Unit = {
    releaseCallback()
  }
}
