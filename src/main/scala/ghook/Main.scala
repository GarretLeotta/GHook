package ghook

import scalafx.Includes._
import scalafx.application.JFXApp
import scalafx.application.JFXApp.PrimaryStage
import scalafx.scene.text.{Text}
import scalafx.scene.{Scene}
import scalafx.scene.layout.{BorderPane}
import scalafx.scene.control.{Button}

import java.lang.reflect.Method;
import java.lang.reflect.Type;

/**
 *
 */
object GHookTest extends JFXApp {
  val globalHook = new GlobalHook(() => println("He took the bait"), () => println("Catch and release"))

  stage = new PrimaryStage {
    title = "GHook test"
    width = 1100
    height = 700
    scene = new Scene {
      content = new BorderPane {
        center = new Text("hello world")
        left = new Button("Register Hook") {
          onAction = _ => println(s"Registering hook: ${globalHook.registerKeyboardHook()}")
        }
        right = new Button("Deregister Hook") {
          onAction = _ => println(s"Deregistering hook: ${globalHook.deregisterKeyboardHook()}")
        }
      }
    }
  }
}
