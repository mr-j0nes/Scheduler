## [0.4.3](https://github.com/mr-j0nes/Scheduler/compare/v0.4.2...v0.4.3) (2026-04-27)


### Bug Fixes

*  fix interval task disabled branch re-queues into recurred_tasks incorrectly ([8b63e9d](https://github.com/mr-j0nes/Scheduler/commit/8b63e9d1594403909b0b92c114274d93f4c56f23))
* add removed guard in private add_task() ([add3d51](https://github.com/mr-j0nes/Scheduler/commit/add3d517e1cc2bfe9063f4c1713a0d069cc1d1c8)), closes [#20](https://github.com/mr-j0nes/Scheduler/issues/20)
* fix shadowed Duration in format_time_point() template ([d66cd1f](https://github.com/mr-j0nes/Scheduler/commit/d66cd1fa7937f1e5634f26291f484d3da8ec0abb))
* use localtime_r() (thread safe) everywhere ([e1355fd](https://github.com/mr-j0nes/Scheduler/commit/e1355fd3e6500316bec9edcec6d122eafb4c3b94))



## [0.4.2](https://github.com/mr-j0nes/Scheduler/compare/v0.4.1...v0.4.2) (2026-04-23)


### Bug Fixes

* remove possible race condition ([ee96613](https://github.com/mr-j0nes/Scheduler/commit/ee9661398221c6f6666a1048ed18e24da67d35bd)), closes [#20](https://github.com/mr-j0nes/Scheduler/issues/20)



## [0.4.1](https://github.com/mr-j0nes/Scheduler/compare/v0.4.0...v0.4.1) (2026-04-20)


### Bug Fixes

* check for task remove when re-adding ([2b4998b](https://github.com/mr-j0nes/Scheduler/commit/2b4998ba0dcb06654b84d89f1c30baf44d71c3e2)), closes [#20](https://github.com/mr-j0nes/Scheduler/issues/20)



# [0.4.0](https://github.com/mr-j0nes/Scheduler/compare/v0.3.4...v0.4.0) (2026-04-17)


### Bug Fixes

* fix format_time_point on VS ([e679dc6](https://github.com/mr-j0nes/Scheduler/commit/e679dc6d4179cbcf8dd2a6b29b3ad8c0d153d30d)), closes [#18](https://github.com/mr-j0nes/Scheduler/issues/18)


### Features

* add has_task() method ([ac1f9f2](https://github.com/mr-j0nes/Scheduler/commit/ac1f9f29e78cded11454dc8e346e4be5f12369de)), closes [#18](https://github.com/mr-j0nes/Scheduler/issues/18)



## [0.3.4](https://github.com/mr-j0nes/Scheduler/compare/v0.3.3...v0.3.4) (2026-04-16)


### Bug Fixes

* remove after removed -> iterator invalidated ([babd1f3](https://github.com/mr-j0nes/Scheduler/commit/babd1f3eb2be81866df6a3766b2b238e1dc24ece))



