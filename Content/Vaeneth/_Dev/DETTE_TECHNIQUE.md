# Déttes Techniques Vaeneth

## Dette Distance Matching (à conserver)

> **Raison :** L'implémentation actuelle du Distance Matching est basée sur des appels répétés à `GetDistanceTo` qui ne sont pas thread-safe. Cela pose problème dans un contexte multi-threadé où plusieurs threads pourraient accéder à la logique de matching simultanément, entraînant des accès mémoire non protégés et des comportements imprévisibles.

> **Action attendue :** Remplacer l'approche actuelle par une implémentation thread-safe qui garantit la cohérence des données partagées. Cela implique soit de synchroniser l'accès aux données critiques via un mutex, soit de restructurer l'architecture pour éviter les accès concurrents.

> **Priorité :** À faire avant Palier 4, car l'architecture de combat devient critique à partir de ce niveau.

---

## Entrées post-audit (2026-09-17)

### 1. ThreadSafeUpdateAnimation
> **Raison :** La fonction `UpdateAnimation` est appelée dans un contexte multi-threadé (via `ThreadSafeUpdateAnimation`) mais ne garantit pas la cohérence des données partagées. Cela pourrait entraîner des accès concurrents non protégés, surtout si plusieurs threads accèdent à des variables critiques (comme `CurrentState`, `CurrentAction`, ou `bIsTransitioning`) en même temps.

> **Action attendue :** Implémenter une synchronisation (mutex ou sémaphore) autour des sections critiques de `UpdateAnimation` pour garantir que seuls un thread à la fois peut modifier les données partagées.

> **Priorité :** À faire avant Palier 4.

---

### 2. AM_Test_RootMotion
> **Raison :** Le montage `AM_Test_RootMotion` est actuellement utilisé pour les tests de Root Motion mais n'a pas encore été intégré à un système de combat réel. Il manque la logique de vérification de l'activation du Root Motion (via `bRootMotionFromMontage`) et la gestion des transitions entre les attaques.

> **Action attendue :** Intégrer `AM_Test_RootMotion` dans le système d'attaque principal avec :
> - Vérification de `bRootMotionFromMontage` avant d'activer Root Motion
> - Gestion des transitions entre les attaques (ex: `TransitionToAttack`)
> - Tests de performance pour s'assurer que Root Motion n'introduit pas de lag

> **Priorité :** À faire avec la première attaque réelle.

---

### 3. Vérification des 7 durées de transition
> **Raison :** Les durées de transition entre les actions (Walk, Idle, Attack, Jump, Fall, Land, Crouch) ont été estimées mais non vérifiées empiriquement. Cela pourrait entraîner des transitions trop longues ou trop brusques, affectant la fluidité du gameplay.

> **Action attendue :** Mesurer les durées réelles de transition entre chaque paire d'actions (Idle→Walk, Walk→Idle, Idle→Attack, Attack→Idle, etc.) à l'aide de `DeltaTime` et de `GetMontageLength()` pour chaque montage. Ajuster les durées pour obtenir une fluidité optimale.

> **Priorité :** À faire avant Palier 4.

---

### 4. Mesure de performance autonome (Standalone) à la fin du Palier 0
> **Raison :** À la fin du Palier 0, l'architecture de combat est encore en développement. Une mesure de performance autonome (standalone) est nécessaire pour évaluer l'impact de l'implémentation actuelle sur la fréquence d'images (FPS), l'utilisation CPU/GPU, et la latence réseau (si applicable). Cela permettra de poser les bases d'un système de monitoring de performance.

> **Action attendue :** Créer un test de performance autonome (standalone) qui :
> - Mesure le FPS pendant l'exécution d'une séquence de combat (Idle→Attack→Idle→Jump→Land)
> - Enregistre l'utilisation CPU/GPU via `UE_LOG` ou un système de profiling interne
> - Enregistre la latence réseau (si applicable)
> - Génère un rapport de performance à la fin du test

> **Priorité :** À faire à la fin du Palier 0.

---

> **Note :** Cette documentation est destinée aux développeurs techniques et sera mise à jour régulièrement en fonction des avancées du projet.
