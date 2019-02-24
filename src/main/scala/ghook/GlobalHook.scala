package ghook

import scala.collection.mutable.Buffer

import scalafx.Includes._
import scalafx.application.Platform
import scalafx.beans.property.ObjectProperty

object GlobalHook {
  sealed trait InputDevice
  case object Keyboard extends InputDevice
  case object Mouse extends InputDevice
  case object NoInput extends InputDevice
  case class KeyBinding(hid: InputDevice, keyCode: Short)
}


/* TODO: Support multiple active hooks */
class GlobalHook(
    pressCallback: () => Unit,
    releaseCallback: () => Unit,
  ) {
  import GlobalHook._
  //Load JNI DLL -- does this work across threads?
  LoadNativeLibraries()

  var hookProp = ObjectProperty[KeyBinding](this, "hookProp")

  @native def registerHook(): Boolean
  @native def deregisterHook(): Boolean

  def changeHook(inputDevice: Char, code: Short): Unit = {
    inputDevice match {
      case 'K' => hookProp.update(KeyBinding(Keyboard, code))
      case 'M' => hookProp.update(KeyBinding(Mouse, code))
      case 'X' => hookProp.update(KeyBinding(NoInput, 0))
      case _ => ()
    }
  }

  def pressFunc(): Unit = {
    pressCallback()
  }
  def releaseFunc(): Unit = {
    releaseCallback()
  }
}
